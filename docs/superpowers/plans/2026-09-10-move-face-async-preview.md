# Move Face Async Preview Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `MoveFaceOp::execute()` (the Move Face tool's Translate/Rotate/Scale/Twist rebuild) off the main thread, so a many-hole body no longer hangs the UI for seconds on every mouse-release, stepper click, or keystroke.

**Architecture:** A single-body scratch-Document worker (`MoveFacePreviewJob`, mirroring the existing `PushPullPreview.h` pattern) copies just the target body, remaps the target face onto the copy, and runs the real `MoveFaceOp::execute()` on a background thread via the existing `AsyncJob<T>` primitive. `MoveFaceController` gains a `PreviewDispatch<MoveFaceKey>` (the same generic state machine `PushPullController` already uses) wired so the expensive path is **always** async from the very first call in a gesture - unlike PushPull/Shell, which measure one inline frame before switching, Move Face's cost is already known (measured, see `movefaceop-freeze` memory) to be too high to ever risk inline. Landing a result is a plain `ctx.doc.updateBody(...)`: Move Face's preview never touches History or a LiveOp instance, and `configureFaceOp` never sets `MoveFaceOp::m_sketchIds` during preview, so the op has no side effect beyond replacing the body shape - no `Precomputed`/`setPrecomputed()` needed on `MoveFaceOp` itself (unlike `PushPullOp`). One deliberate deviation from the Push/Pull precedent (Codex review rounds 2-3): `PreviewDispatch::shouldLaunch()`'s busy-gate (refuse to launch while a job is already running) is right for Push/Pull, whose `update()` runs every frame and wants to wait for the current job before deciding whether to relaunch; Move Face's four trigger sites are discrete events, not per-frame, so that all-or-nothing gate drops legitimate requests. But launching unconditionally is also wrong (round 3, finding 1): `AsyncJob::abandon()` doesn't cancel a running thread, so rapid events during one still-running multi-second computation would pile up concurrent rebuilds with no cap. The final design keeps at most ONE worker in flight (checked directly via `AsyncJob::running()`, not `PreviewDispatch::shouldLaunch()`) plus a single `m_mfPendingRelaunch` flag remembering that a real request arrived while busy, retried once the worker frees up. `PreviewDispatch` is used only for `launched()`/`finished()`'s key-matching, never `shouldLaunch()`.

**Tech Stack:** C++17, OCCT 7.9.3, GoogleTest, CMake. No new third-party dependencies.

**Spec:** This plan document. The problem statement and all measured numbers live in `~/.claude/projects/-Users-laptop-Documents-Coding-projects-Materialzr/memory/movefaceop-freeze.md` (read it before starting - it also documents two corrections made while investigating, both binding on this plan: the cost is per-*event*, not per-frame, and `MoveFaceOp::serializeParams()` is NOT a safe dispatch key).

## Global Constraints

- Do not change the MODELING behavior of `commitMoveFace()`, `cancelMoveFace()`, hole-move mode (`m_st.moveHoleMode`), or the local-tweak path (`localTweakApplies()`/`applyLocalTweak()`) - all four already run cheap ops (`MoveHoleOp`, `FaceTweakOp`) or a full History commit, none showed up in the bench, and none are in scope. (Corrected in Codex review round 2, finding 6: `commitMoveFace()`/`cancelMoveFace()` DO get two lines of async-lifecycle bookkeeping each - `m_mfJob.abandon(); m_mfDispatch.reset();` - so an in-flight job doesn't outlive the gesture; that is not a modeling change and is required by finding 4.)
- Only the "general" branch of `MoveFaceController::updateMoveFace()` - the one that builds a `MoveFaceOp` and calls `execute()` - is being made async (`src/app/FaceOpControllers.cpp:1639-1652` as of this plan).
- Reuse `AsyncJob<T>` (`src/app/AsyncJob.h`) and `PreviewDispatch<Key>` (`src/app/PreviewDispatch.h`) as-is. Do not modify either class.
- Do NOT add `Precomputed`/`setPrecomputed()` to `MoveFaceOp`. Verify this claim in Task 1's tests before relying on it further (see Task 1, Step 4).
- Do NOT reuse `MoveFaceOp::serializeParams()` as a dispatch key (it round-trips only Translate/Twist parameters; Rotate's explicit transform and all of Scale are absent - see the reload-format comment at `MoveFaceOp.cpp:603-606`).
- Every new/changed test file: run the full suite from `build/` with `ctest --output-on-failure` UNSANDBOXED after each task (project memory: sandboxed ctest fails 5-6 unrelated file-IO suites on denied `/tmp` writes - exclude `reload_edit|stl_import|topo_boolean_gen|full_replay|svg_roundtrip|test_project_thumbnail` if running sandboxed, or just run unsandboxed). No test count may drop, no new failures.
- No `Co-Authored-By: Claude` trailer on any commit in this repo (confirmed project convention).
- Before writing any implementation code for Task 2 or later, this plan must clear `claudex-loop:codex-review` (CLAUDE.md hard rule - every prior async-preview conversion in this repo went through this gate, and two of them found real concurrency bugs in the design before code was written).

---

### Task 1: `MoveFacePreviewJob` - the off-thread worker

**Files:**
- Create: `src/app/MoveFaceDispatch.h`
- Create: `src/app/MoveFacePreview.h`
- Create: `src/app/MoveFacePreview.cpp`
- Modify: `tests/CMakeLists.txt` (add `MoveFacePreview.cpp` to `materializr_core`'s source list, register `test_move_face_preview`)
- Test: `tests/test_move_face_preview.cpp`

**Interfaces:**
- Produces: `materializr::MoveFaceKey` (value type, `operator==`), `materializr::MoveFacePreviewResult { bool ok; TopoDS_Shape shape; double millis; }`, `materializr::MoveFacePreviewJob` with `static std::unique_ptr<MoveFacePreviewJob> prepare(const TopoDS_Shape& originalBody, const TopoDS_Face& face, const std::function<void(MoveFaceOp&)>& configure)` and `MoveFacePreviewResult run()`.
- Consumes: `MoveFaceOp` (`src/modeling/MoveFaceOp.h`, unchanged), `Document` (`src/core/Document.h`), `AsyncJob<T>` is NOT used in this task - that's Task 2.

- [ ] **Step 1: Write `MoveFaceDispatch.h` with the key struct**

```cpp
#pragma once

#include "MoveFaceState.h" // FaceXform
#include <glm/glm.hpp>

namespace materializr {

// Everything a Move Face preview call varies frame-to-frame (NOT loop-motion
// or which face/body - those are fixed for the whole gesture, set once at
// begin). Deliberately hand-built rather than derived from
// MoveFaceOp::serializeParams(): that string is a RELOAD format that only
// round-trips Translate/Twist (see MoveFaceOp.cpp's own comment on
// serializeParams), so it silently ignores Rotate's transform and all of
// Scale - two frames with different rotations would compare equal and the
// dispatch would show a stale result forever.
//
// MUST stay in sync with MoveFaceController::configureFaceOp(): every field
// that function reads from MoveFaceState to configure the op belongs here
// too. There is no compiler check for this; a new configureFaceOp parameter
// needs a new field here in the same change.
//
// Deliberately does NOT include m_st.moveFaceLocal: that flag doesn't change
// what MoveFaceOp computes, it changes whether MoveFaceOp runs at all
// (localTweakApplies() routes to FaceTweakOp instead). MoveFaceController's
// pollPreview() discards any landed general-path result while
// localTweakApplies() is true, precisely so a stale key match can never
// overwrite a local rebuild - do not "fix" that by adding the flag here.
struct MoveFaceKey {
    // Round 4 addition: a cross-gesture safety net, not something
    // configureFaceOp reads. If a job launched by gesture N is still
    // computing when gesture N+1 begins on a DIFFERENT body,
    // MoveFaceController::pollPreview() must never write gesture N's result
    // onto gesture N+1's body id. beginMoveFace() resets m_mfDispatch, which
    // already makes this vanishingly unlikely on its own (the reset key
    // essentially never matches a real gesture's key) - this field makes it
    // impossible rather than merely unlikely.
    int bodyId = -1;
    FaceXform kind = FaceXform::Translate;
    bool isTwist = false;
    glm::vec3 moveVec{0.0f};       // Translate
    glm::mat3 rotMat{1.0f};        // Rotate: faceRotTotal()'s output
    glm::vec3 pivot{0.0f};         // Rotate/Scale pivot (fixed per gesture, kept for safety)
    float twistAngle = 0.0f;       // Twist
    bool scaleUniform = true;
    float scaleFactor = 1.0f;      // Scale, uniform
    float scaleA = 1.0f, scaleB = 1.0f; // Scale, non-uniform
    glm::vec3 scaleAxisA{1.0f, 0.0f, 0.0f}, scaleAxisB{0.0f, 1.0f, 0.0f};

    bool operator==(const MoveFaceKey& o) const
    {
        return bodyId == o.bodyId && kind == o.kind && isTwist == o.isTwist &&
               moveVec == o.moveVec && rotMat == o.rotMat && pivot == o.pivot &&
               twistAngle == o.twistAngle && scaleUniform == o.scaleUniform &&
               scaleFactor == o.scaleFactor && scaleA == o.scaleA && scaleB == o.scaleB &&
               scaleAxisA == o.scaleAxisA && scaleAxisB == o.scaleAxisB;
    }
};

} // namespace materializr
```

- [ ] **Step 2: Write the failing test for `MoveFacePreviewJob`**

Create `tests/test_move_face_preview.cpp`:

```cpp
// MoveFacePreviewJob runs MoveFaceOp::execute() on a scratch copy of ONE
// body and reports the resulting shape. Unlike PushPullPreview there is no
// Precomputed/setPrecomputed step: MoveFaceOp's preview configuration never
// sets m_sketchIds, so execute() on a preview call has no side effect beyond
// doc.updateBody() - the caller lands the reported shape directly.
#include "app/MoveFacePreview.h"
#include "core/Document.h"
#include "modeling/MoveFaceOp.h"

#include <gtest/gtest.h>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>

using materializr::MoveFacePreviewJob;
using materializr::MoveFacePreviewResult;

namespace {

double volume(const TopoDS_Shape& s) {
    GProp_GProps g; BRepGProp::VolumeProperties(s, g); return g.Mass();
}

// Small multi-hole plate: enough holes to exercise the per-hole loft+cut
// path (see buildFeature in MoveFaceOp.cpp), cheap enough to run in a unit
// test (the perf table lives in movefaceop-freeze memory, not here).
TopoDS_Shape makeHolePlate(int n, double pitch = 15.0, double r = 3.0,
                           double thickness = 5.0) {
    const double side = (n + 1) * pitch;
    BRepBuilderAPI_MakePolygon poly;
    poly.Add(gp_Pnt(0, 0, 0));
    poly.Add(gp_Pnt(side, 0, 0));
    poly.Add(gp_Pnt(side, side, 0));
    poly.Add(gp_Pnt(0, side, 0));
    poly.Close();
    TopoDS_Shape plate = BRepPrimAPI_MakePrism(
        BRepBuilderAPI_MakeFace(poly.Wire()).Face(), gp_Vec(0, 0, thickness)).Shape();
    TopoDS_Compound holes;
    BRep_Builder bb;
    bb.MakeCompound(holes);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            bb.Add(holes, BRepPrimAPI_MakeCylinder(
                gp_Ax2(gp_Pnt(pitch * (i + 1), pitch * (j + 1), -1.0), gp_Dir(0, 0, 1)),
                r, thickness + 2.0).Shape());
    return BRepAlgoAPI_Cut(plate, holes).Shape();
}

TopoDS_Face topFace(const TopoDS_Shape& s) {
    TopoDS_Face best; double bestZ = -1e300;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() > bestZ) { bestZ = g.CentreOfMass().Z(); best = TopoDS::Face(e.Current()); }
    }
    return best;
}

} // namespace

TEST(MoveFacePreview, TranslateMatchesADirectExecuteAndLeavesTheLiveDocumentAlone) {
    TopoDS_Shape body = makeHolePlate(3); // 9 holes: enough to hit buildFeature's loop
    TopoDS_Face face = topFace(body);
    ASSERT_FALSE(face.IsNull());

    auto configure = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Translate);
        op.setMoveVector(gp_Vec(1.0, 0.5, 0.0));
        op.setLoopMotion(true, std::vector<bool>(9, false), std::vector<bool>(9, false));
    };

    std::unique_ptr<MoveFacePreviewJob> job = MoveFacePreviewJob::prepare(body, face, configure);
    ASSERT_TRUE(job);
    MoveFacePreviewResult r = job->run();
    ASSERT_TRUE(r.ok);
    EXPECT_FALSE(r.shape.IsNull());

    // Reference: the same gesture run directly on a live Document.
    Document live;
    int id = live.addBody(body, "plate");
    MoveFaceOp direct;
    direct.setBody(id);
    direct.setFace(face);
    configure(direct);
    ASSERT_TRUE(direct.execute(live));
    EXPECT_NEAR(volume(r.shape), volume(live.getBody(id)), 1e-6);

    // The ORIGINAL shape (what `body` pointed to before either call) must be
    // untouched: the worker only ever wrote to its own scratch Document.
    EXPECT_NEAR(volume(body), volume(makeHolePlate(3)), 1e-6);
}

TEST(MoveFacePreview, RotateAndScaleAlsoMatchADirectExecute) {
    TopoDS_Shape body = makeHolePlate(2);
    TopoDS_Face face = topFace(body);

    auto configureRotate = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Rotate);
        op.setRotation(gp_Dir(1, 0, 0), 0.05);
        op.setLoopMotion(true, std::vector<bool>(4, false), std::vector<bool>(4, false));
    };
    std::unique_ptr<MoveFacePreviewJob> rotJob = MoveFacePreviewJob::prepare(body, face, configureRotate);
    ASSERT_TRUE(rotJob);
    MoveFacePreviewResult rr = rotJob->run();
    ASSERT_TRUE(rr.ok);

    Document liveR;
    int idR = liveR.addBody(body, "plate");
    MoveFaceOp directR;
    directR.setBody(idR);
    directR.setFace(face);
    configureRotate(directR);
    ASSERT_TRUE(directR.execute(liveR));
    EXPECT_NEAR(volume(rr.shape), volume(liveR.getBody(idR)), 1e-6);

    auto configureScale = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Scale);
        op.setScaleFactor(1.1);
        op.setLoopMotion(true, std::vector<bool>(4, false), std::vector<bool>(4, false));
    };
    std::unique_ptr<MoveFacePreviewJob> sclJob = MoveFacePreviewJob::prepare(body, face, configureScale);
    ASSERT_TRUE(sclJob);
    MoveFacePreviewResult sr = sclJob->run();
    ASSERT_TRUE(sr.ok);

    Document liveS;
    int idS = liveS.addBody(body, "plate");
    MoveFaceOp directS;
    directS.setBody(idS);
    directS.setFace(face);
    configureScale(directS);
    ASSERT_TRUE(directS.execute(liveS));
    EXPECT_NEAR(volume(sr.shape), volume(liveS.getBody(idS)), 1e-6);
}

// Round 3 finding 2: the round-2 summary claimed "all four kinds" were
// covered, but only setRotation() and uniform setScaleFactor() were
// exercised - never setRotationExplicit() (what configureFaceOp actually
// calls for Rotate), setScaleNonUniform(), or setTwist() at all. Each is a
// DIFFERENT branch inside MoveFaceOp::execute() (m_rotUseExplicit,
// m_scaleNonUniform), so a bug specific to one could hide behind the other.
TEST(MoveFacePreview, ExplicitRotationNonUniformScaleAndTwistAlsoMatchADirectExecute) {
    TopoDS_Shape body = makeHolePlate(2);
    TopoDS_Face face = topFace(body);

    gp_Trsf explicitRot;
    explicitRot.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), 0.03);
    auto configureExplicitRotate = [&](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Rotate);
        op.setRotationExplicit(explicitRot);
        op.setLoopMotion(true, std::vector<bool>(4, false), std::vector<bool>(4, false));
    };
    std::unique_ptr<MoveFacePreviewJob> rotJob = MoveFacePreviewJob::prepare(body, face, configureExplicitRotate);
    ASSERT_TRUE(rotJob);
    MoveFacePreviewResult rr = rotJob->run();
    ASSERT_TRUE(rr.ok);
    Document liveR;
    int idR = liveR.addBody(body, "plate");
    MoveFaceOp directR;
    directR.setBody(idR);
    directR.setFace(face);
    configureExplicitRotate(directR);
    ASSERT_TRUE(directR.execute(liveR));
    EXPECT_NEAR(volume(rr.shape), volume(liveR.getBody(idR)), 1e-6);

    auto configureNonUniformScale = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Scale);
        op.setScaleNonUniform(gp_Dir(1, 0, 0), gp_Dir(0, 1, 0), 1.2, 0.9);
        op.setLoopMotion(true, std::vector<bool>(4, false), std::vector<bool>(4, false));
    };
    std::unique_ptr<MoveFacePreviewJob> sclJob = MoveFacePreviewJob::prepare(body, face, configureNonUniformScale);
    ASSERT_TRUE(sclJob);
    MoveFacePreviewResult sr = sclJob->run();
    ASSERT_TRUE(sr.ok);
    Document liveS;
    int idS = liveS.addBody(body, "plate");
    MoveFaceOp directS;
    directS.setBody(idS);
    directS.setFace(face);
    configureNonUniformScale(directS);
    ASSERT_TRUE(directS.execute(liveS));
    EXPECT_NEAR(volume(sr.shape), volume(liveS.getBody(idS)), 1e-6);

    auto configureTwist = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Twist);
        op.setTwist(0.1);
        op.setLoopMotion(true, std::vector<bool>(4, false), std::vector<bool>(4, false));
    };
    std::unique_ptr<MoveFacePreviewJob> twJob = MoveFacePreviewJob::prepare(body, face, configureTwist);
    ASSERT_TRUE(twJob);
    MoveFacePreviewResult tr = twJob->run();
    ASSERT_TRUE(tr.ok);
    Document liveT;
    int idT = liveT.addBody(body, "plate");
    MoveFaceOp directT;
    directT.setBody(idT);
    directT.setFace(face);
    configureTwist(directT);
    ASSERT_TRUE(directT.execute(liveT));
    EXPECT_NEAR(volume(tr.shape), volume(liveT.getBody(idT)), 1e-6);
}

TEST(MoveFacePreview, AFaceThatIsNotPartOfTheGivenBodyRefusesInsteadOfCrashing) {
    TopoDS_Shape bodyA = makeHolePlate(2);
    TopoDS_Shape bodyB = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();
    TopoDS_Face foreignFace = topFace(bodyB); // not a sub-shape of bodyA

    auto configure = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Translate);
        op.setMoveVector(gp_Vec(1.0, 0.0, 0.0));
    };
    std::unique_ptr<MoveFacePreviewJob> job = MoveFacePreviewJob::prepare(bodyA, foreignFace, configure);
    EXPECT_FALSE(job); // BRepBuilderAPI_Copy::ModifiedShape would throw; prepare() must catch it
}

TEST(MoveFacePreview, ANoOpGestureRefusesRatherThanReturningTheUnchangedBody) {
    TopoDS_Shape body = makeHolePlate(2);
    TopoDS_Face face = topFace(body);
    auto configure = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Translate);
        op.setMoveVector(gp_Vec(0.0, 0.0, 0.0)); // MoveFaceOp::execute's own guard: magnitude < 1e-6
    };
    std::unique_ptr<MoveFacePreviewJob> job = MoveFacePreviewJob::prepare(body, face, configure);
    ASSERT_TRUE(job); // prepare() cannot know the gesture is a no-op ahead of run()
    MoveFacePreviewResult r = job->run();
    EXPECT_FALSE(r.ok); // execute() returns false; the controller must not adopt r.shape
}
```

- [ ] **Step 2b: Register the test and source file in CMake - BOTH targets**

This codebase double-compiles most `src/app/*.cpp` files: the app executable (`add_executable(materializr ...)` in the ROOT `CMakeLists.txt`) has its own explicit source list and does NOT link `materializr_core` - `materializr_core` is a second, separate static-library build of a subset of the same sources, built only for headless tests. `PushPullPreview.cpp`, `SnapshotPreview.cpp`, `GhostMesh.cpp` etc. all already appear in BOTH lists (confirmed at `CMakeLists.txt:205-230` and `tests/CMakeLists.txt:23-125`). Missing either one is a real, silent bug: skip the root list and the shipped app fails to link (`MoveFacePreviewJob` referenced from `FaceOpControllers.cpp` but never compiled into `materializr`); skip the `materializr_core` list and every test that touches it fails to link instead.

In `CMakeLists.txt` (root), add `src/app/MoveFacePreview.cpp` to the `add_executable(materializr ...)` source list, next to `src/app/PushPullPreview.cpp`.

In `tests/CMakeLists.txt`, add `${CMAKE_SOURCE_DIR}/src/app/MoveFacePreview.cpp` to the `materializr_core` source list (near the other `src/app/*.cpp` entries: `PushPullPreview.cpp`, `SnapshotPreview.cpp`), and near `test_pushpull_preview`'s registration add:

```cmake
add_executable(test_move_face_preview test_move_face_preview.cpp)
target_link_libraries(test_move_face_preview PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_move_face_preview COMMAND test_move_face_preview)
```

- [ ] **Step 3: Run the test to verify it fails to compile / link**

Run: `cmake --build build --target test_move_face_preview -j 8`
Expected: FAIL - `app/MoveFacePreview.h` does not exist yet.

- [ ] **Step 4: Write `MoveFacePreview.h`**

```cpp
#pragma once

#include "core/Document.h"

#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <functional>
#include <memory>

class MoveFaceOp;

namespace materializr {

// Runs a Move Face preview's real MoveFaceOp::execute() off the main
// thread. Unlike PushPullPreview there is exactly ONE body involved (Move
// Face never touches a second body) and no Precomputed/setPrecomputed step:
// `configure` never calls MoveFaceOp::setSketchIds() during a preview (only
// commitMoveFace() does, on the live document), so execute() has no effect
// beyond replacing the scratch document's body - run() just hands that shape
// back for the caller to land with a plain Document::updateBody().
struct MoveFacePreviewResult {
    bool ok = false;
    TopoDS_Shape shape;
    double millis = 0.0; // execute() wall time on the worker, for diagnostics
};

class MoveFacePreviewJob {
public:
    // Main thread. `originalBody` is the gesture's snapshot (NOT the live
    // document - the live document may carry a stale preview from a prior
    // frame). `configure` sets kind/vector/rotation/scale/twist/loop-motion
    // on the scratch op; pass MoveFaceController::configureFaceOp bound to
    // the controller. Null when the face is not a live sub-shape of
    // originalBody (BRepBuilderAPI_Copy::ModifiedShape would throw).
    static std::unique_ptr<MoveFacePreviewJob> prepare(
        const TopoDS_Shape& originalBody, const TopoDS_Face& face,
        const std::function<void(MoveFaceOp&)>& configure);
    ~MoveFacePreviewJob();

    // Worker thread. Safe to call exactly once.
    MoveFacePreviewResult run();

private:
    MoveFacePreviewJob();
    std::unique_ptr<Document> m_scratch;
    int m_scratchBodyId = -1;
    std::unique_ptr<MoveFaceOp> m_op;
};

} // namespace materializr
```

- [ ] **Step 5: Write `MoveFacePreview.cpp`**

```cpp
#include "MoveFacePreview.h"

#include "modeling/MoveFaceOp.h"

#include <BRepBuilderAPI_Copy.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>

#include <chrono>

namespace materializr {

MoveFacePreviewJob::MoveFacePreviewJob() = default;
MoveFacePreviewJob::~MoveFacePreviewJob() = default;

std::unique_ptr<MoveFacePreviewJob> MoveFacePreviewJob::prepare(
    const TopoDS_Shape& originalBody, const TopoDS_Face& face,
    const std::function<void(MoveFaceOp&)>& configure)
{
    if (originalBody.IsNull() || face.IsNull()) return nullptr;
    try {
        TopTools_IndexedMapOfShape faces;
        TopExp::MapShapes(originalBody, TopAbs_FACE, faces);
        if (!faces.Contains(face)) return nullptr; // stale handle - refuse, don't guess

        BRepBuilderAPI_Copy copier;
        copier.Perform(originalBody, Standard_True, Standard_False);
        const TopoDS_Shape scratchFace = copier.ModifiedShape(face); // throws if not a sub-shape
        if (scratchFace.IsNull() || scratchFace.ShapeType() != TopAbs_FACE) return nullptr;

        std::unique_ptr<MoveFacePreviewJob> job(new MoveFacePreviewJob());
        job->m_scratch = std::make_unique<Document>();
        job->m_scratchBodyId = job->m_scratch->addBody(copier.Shape(), "preview");
        job->m_op = std::make_unique<MoveFaceOp>();
        job->m_op->setBody(job->m_scratchBodyId);
        job->m_op->setFace(TopoDS::Face(scratchFace));
        configure(*job->m_op);
        return job;
    } catch (...) {
        return nullptr; // a refused copy or a stale sub-shape: not previewed off-thread
    }
}

MoveFacePreviewResult MoveFacePreviewJob::run()
{
    MoveFacePreviewResult r;
    if (!m_scratch || !m_op) return r;
    const auto t0 = std::chrono::steady_clock::now();
    try {
        r.ok = m_op->execute(*m_scratch);
    } catch (...) {
        r.ok = false;
    }
    r.millis = std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - t0).count();
    if (!r.ok) return r;
    try {
        r.shape = m_scratch->getBody(m_scratchBodyId);
    } catch (...) {
        r.ok = false;
    }
    return r;
}

} // namespace materializr
```

- [ ] **Step 6: Build and run the test**

Run: `cmake --build build --target test_move_face_preview -j 8 && ./build/tests/test_move_face_preview`
Expected: PASS, all 4 cases.

- [ ] **Step 7: Run the full suite**

Run (unsandboxed): `cd build && ctest --output-on-failure`
Expected: every existing test still passes, plus `test_move_face_preview`.

- [ ] **Step 8: Commit**

```bash
git add src/app/MoveFaceDispatch.h src/app/MoveFacePreview.h src/app/MoveFacePreview.cpp tests/test_move_face_preview.cpp tests/CMakeLists.txt
git commit -m "app: Add MoveFacePreviewJob, the off-thread worker for Move Face"
```

---

### Task 2: Wire the worker into `MoveFaceController`

**Files:**
- Modify: `src/app/FaceOpControllers.h` (add members, override `pollPreview`/`previewPending`, declare `currentMoveFaceKey`, `launchMoveFacePreviewIfWanted`)
- Modify: `src/app/FaceOpControllers.cpp` (implement the above; rewrite the general branch of `updateMoveFace`; reset dispatch state in `beginMoveFace`)
- Modify: `tests/CMakeLists.txt` (add `src/app/FaceOpControllers.cpp` AND `src/app/CylindricalPick.cpp` to the `materializr_core` source list - `FaceOpControllers.cpp` is currently compiled only into the app's own executable target, and this task's test is the first thing in the repo to need `MoveFaceController` headlessly; `CylindricalPick.cpp` is a hard link dependency of it, confirmed via `detectCylindricalPick()` at `FaceOpControllers.cpp:906` - Codex review round 2, finding 4. `UserAxes.h`, also included by `FaceOpControllers.cpp`, is header-only and needs nothing added. Register `test_move_face_async`)
- Test: `tests/test_move_face_async.cpp`

**Interfaces:**
- Consumes: `MoveFacePreviewJob`/`MoveFacePreviewResult` (Task 1), `MoveFaceKey` (Task 1), `AsyncJob<T>` (`src/app/AsyncJob.h`, unmodified), `PreviewDispatch<Key>` (`src/app/PreviewDispatch.h`, unmodified), `IopContext` (`src/app/InteractiveOpController.h`, unmodified).
- Produces: `MoveFaceController::pollPreview(const IopContext&) override`, `MoveFaceController::previewPending() const override` - both already declared virtual on the base with the exact signatures used by `PushPullController`.

- [ ] **Step 1: Write the failing headless test**

No prior test constructs a full `IopContext` for `MoveFaceController` (the project's own controller-level tests, e.g. `test_iop_latch.cpp`, use a stub that never reaches `IopContext`). Build the minimal real one here - every `std::function` member is required by the struct even when this test never calls it, so unused ones get empty/no-op lambdas.

Create `tests/test_move_face_async.cpp`:

```cpp
// MoveFaceController's general (Translate/Rotate/Scale/Twist) preview path
// must never block the calling thread for the op's real cost: it launches a
// MoveFacePreviewJob and returns immediately, landing the result once
// pollPreview() sees it. See movefaceop-freeze memory for why this matters -
// the op is O(n^1.6) in hole count and reaches multiple seconds well before
// "many holes" fixtures used elsewhere in this project's perf work.
#include "app/FaceOpControllers.h"
#include "app/InteractiveOpController.h"
#include "core/Document.h"
#include "core/History.h"
#include "core/SelectionManager.h"
#include "modeling/MoveFaceOp.h"

#include <gtest/gtest.h>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>

#include <chrono>
#include <thread>

using namespace materializr;

namespace {

double volume(const TopoDS_Shape& s) {
    GProp_GProps g; BRepGProp::VolumeProperties(s, g); return g.Mass();
}

// A Translate is volume-preserving (round 1 finding 7: the original body,
// a stale preview, and the correct one can all have the SAME volume), so
// every test below also checks WHERE the top face ended up, not just how
// much material there is.
double topFaceCentroidX(const TopoDS_Shape& s) {
    TopoDS_Face best; double bestZ = -1e300;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() > bestZ) { bestZ = g.CentreOfMass().Z(); best = TopoDS::Face(e.Current()); }
    }
    GProp_GProps g; BRepGProp::SurfaceProperties(best, g);
    return g.CentreOfMass().X();
}

// A Rotate pivots about the face's OWN centroid (see configureFaceOp), so
// topFaceCentroidX above is useless for it - rotation about a point does
// not move that point. This measures the top face's Z-extent (how far out
// of its original horizontal plane it now reads), which scales with both
// angle and distance from the pivot and so DOES discriminate between two
// different rotation angles.
double topFaceZSpread(const TopoDS_Shape& s) {
    TopoDS_Face best; double bestZ = -1e300;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() > bestZ) { bestZ = g.CentreOfMass().Z(); best = TopoDS::Face(e.Current()); }
    }
    Bnd_Box box;
    BRepBndLib::Add(best, box);
    double xmn, ymn, zmn, xmx, ymx, zmx;
    box.Get(xmn, ymn, zmn, xmx, ymx, zmx);
    return zmx - zmn;
}

// A body heavy enough to prove "did not block" without a multi-second unit
// test: the perf CURVE is already established in movefaceop-freeze memory
// (25 holes = 99.6ms is already 3x a 30ms frame budget), so this only needs
// to be slow enough that a synchronous call would fail a tight wall-clock
// assertion, not slow enough to reproduce the worst case.
TopoDS_Shape makeHolePlate(int n) {
    const double pitch = 15.0, r = 3.0, thickness = 5.0;
    const double side = (n + 1) * pitch;
    BRepBuilderAPI_MakePolygon poly;
    poly.Add(gp_Pnt(0, 0, 0));
    poly.Add(gp_Pnt(side, 0, 0));
    poly.Add(gp_Pnt(side, side, 0));
    poly.Add(gp_Pnt(0, side, 0));
    poly.Close();
    TopoDS_Shape plate = BRepPrimAPI_MakePrism(
        BRepBuilderAPI_MakeFace(poly.Wire()).Face(), gp_Vec(0, 0, thickness)).Shape();
    TopoDS_Compound holes;
    BRep_Builder bb;
    bb.MakeCompound(holes);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            bb.Add(holes, BRepPrimAPI_MakeCylinder(
                gp_Ax2(gp_Pnt(pitch * (i + 1), pitch * (j + 1), -1.0), gp_Dir(0, 0, 1)),
                r, thickness + 2.0).Shape());
    return BRepAlgoAPI_Cut(plate, holes).Shape();
}

TopoDS_Face topFace(const TopoDS_Shape& s) {
    TopoDS_Face best; double bestZ = -1e300;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() > bestZ) { bestZ = g.CentreOfMass().Z(); best = TopoDS::Face(e.Current()); }
    }
    return best;
}

// The minimal real IopContext: every callback the struct requires, wired to
// a real Document/History/SelectionManager where the test needs one and a
// no-op everywhere MoveFaceController's Translate/Rotate/Scale/Twist path
// never calls it (hole-move and local-tweak specific callbacks - toast,
// refuseMesh, sketchForBody, ghost preview, panel placement).
struct Harness {
    Document doc;
    History history; // default-constructed; pushOperation takes doc per call, not at construction
    SelectionManager selection;
    int meshDirtyCalls = 0;

    IopContext ctx() {
        return IopContext{
            doc, history, selection,
            [this] { ++meshDirtyCalls; },              // markMeshesDirty
            [](float, const char*) { return false; },  // progress
            [](std::function<void()> f) { f(); },      // deferHeavy (run inline; unused here)
            false,                                     // cornerCommitUi
            [](const char*) {},                        // toast
            [](const char*) { return false; },         // refuseMesh
            false, 0.0f,                                // snapToGrid, gridStep
            IopPanelPlace{},                            // panel
            [](int) {},                                 // ensureSketchSourceFace
            [](const TopoDS_Face&, const gp_Pln&) { return -1; }, // findBodyUnderRegion
            [](int) {},                                  // markBodyDirty
            [](int) { return false; },                   // bodyHasRenderSlot
            [](const TopoDS_Shape&, bool) {},             // showGhost
            [] {},                                        // clearGhost
            [](int) { return -1; },                        // sketchForBody
            [](const std::vector<float>&, bool) {},         // showGhostMesh
        };
    }
};

} // namespace

TEST(MoveFaceAsyncPreview, ReleaseAfterADragReturnsWellUnderTheOpsRealCost) {
    Harness h;
    TopoDS_Shape body = makeHolePlate(5); // 25 holes: ~100ms inline per the perf table
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Translate;
    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.5f, 0.0f);
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);

    // Round 1 finding 7: a fixed absolute threshold is machine-dependent.
    // Measure the REAL inline cost of the identical gesture in this same
    // run and assert the async call is a small fraction of it - true on any
    // machine, and exactly the property that matters (the call must not
    // block for anywhere close to the op's real cost).
    const auto d0 = std::chrono::steady_clock::now();
    {
        Document timing;
        int tId = timing.addBody(body, "plate");
        MoveFaceOp direct;
        direct.setBody(tId);
        direct.setFace(topFace(timing.getBody(tId)));
        direct.setKind(MoveFaceOp::Kind::Translate);
        direct.setMoveVector(gp_Vec(1.0, 0.5, 0.0));
        direct.setLoopMotion(true, std::vector<bool>(25, false), std::vector<bool>(25, false));
        ASSERT_TRUE(direct.execute(timing));
    }
    const double directMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - d0).count();
    ASSERT_GT(directMs, 10.0) << "fixture too cheap to prove anything - raise the hole count";

    const auto t0 = std::chrono::steady_clock::now();
    mfc.updateMoveFace(ctx); // the mouse-release call site
    const double callMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_LT(callMs, directMs * 0.2)
        << "updateMoveFace() must launch a worker and return, not run the "
           "op (" << directMs << "ms measured this run) inline";
    EXPECT_TRUE(mfc.previewPending());

    // Poll until the worker lands (bounded: a real hang here is a test bug,
    // not a product one - the whole point of Task 1 is that this finishes).
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_FALSE(mfc.previewPending()) << "worker never landed within 5s";

    // Reference: the same gesture run directly.
    Document ref;
    int refId = ref.addBody(body, "plate");
    MoveFaceOp direct;
    direct.setBody(refId);
    direct.setFace(topFace(ref.getBody(refId)));
    direct.setKind(MoveFaceOp::Kind::Translate);
    direct.setMoveVector(gp_Vec(1.0, 0.5, 0.0));
    direct.setLoopMotion(true, std::vector<bool>(25, false), std::vector<bool>(25, false));
    ASSERT_TRUE(direct.execute(ref));

    EXPECT_NEAR(volume(h.doc.getBody(bodyId)), volume(ref.getBody(refId)), 1e-6);
    EXPECT_NEAR(topFaceCentroidX(h.doc.getBody(bodyId)), topFaceCentroidX(ref.getBody(refId)), 1e-6);
}

TEST(MoveFaceAsyncPreview, ASupersedingCallBeforeLandingDropsTheStaleResult) {
    Harness h;
    TopoDS_Shape body = makeHolePlate(5);
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Translate;
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);

    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.0f, 0.0f);
    mfc.updateMoveFace(ctx); // launches job A
    ASSERT_TRUE(mfc.previewPending()) << "job A must actually have launched";
    mfc.st().moveFaceVec = glm::vec3(2.0f, 0.0f, 0.0f);
    mfc.updateMoveFace(ctx); // arrow moved before A landed: must ask again at the NEW key
    EXPECT_TRUE(mfc.previewPending()) << "job B must be in flight (A may still be finishing)";

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_FALSE(mfc.previewPending());

    Document ref;
    int refId = ref.addBody(body, "plate");
    MoveFaceOp direct;
    direct.setBody(refId);
    direct.setFace(topFace(ref.getBody(refId)));
    direct.setKind(MoveFaceOp::Kind::Translate);
    direct.setMoveVector(gp_Vec(2.0, 0.0, 0.0)); // the FINAL vector, not the superseded one
    direct.setLoopMotion(true, std::vector<bool>(25, false), std::vector<bool>(25, false));
    ASSERT_TRUE(direct.execute(ref));

    EXPECT_NEAR(volume(h.doc.getBody(bodyId)), volume(ref.getBody(refId)), 1e-6);
    EXPECT_NEAR(topFaceCentroidX(h.doc.getBody(bodyId)), topFaceCentroidX(ref.getBody(refId)), 1e-6)
        << "landing must reflect vec=2.0, not the superseded vec=1.0";
}

TEST(MoveFaceAsyncPreview, CommitAbandonsAnInFlightJobInsteadOfLeavingItAppliedLater) {
    // Round 1 finding 4 (pollPreview must drain a job regardless of gesture
    // state) as corrected by round 4 (abandon() does NOT stop the thread,
    // so previewPending() correctly stays true until it genuinely
    // finishes - the property under test is that the result never gets
    // applied after commit, and previewPending() eventually clears once the
    // real computation is done, not that it clears instantly).
    Harness h;
    TopoDS_Shape body = makeHolePlate(5); // ~100ms per the perf table - bounds the wait below
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Translate;
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);
    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.0f, 0.0f);

    mfc.updateMoveFace(ctx);
    ASSERT_TRUE(mfc.previewPending());
    mfc.commitMoveFace(ctx); // must not block waiting for the job
    const TopoDS_Shape afterCommit = h.doc.getBody(bodyId);

    // Round 5 finding 2: previewPending() no longer counts an abandoned
    // job (see its definition), so it goes false right away here - that is
    // correct, not the thing under test. What matters is that the
    // abandoned job's result never lands once it actually finishes; poll
    // for a fixed, generous duration UNCONDITIONALLY (not gated on
    // previewPending(), which tells us nothing about the abandoned job any
    // more) to give it time to complete and be reaped.
    EXPECT_FALSE(mfc.previewPending())
        << "abandon() must not leave previewPending() true - nothing is being waited for any more";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Verify document integrity after the abandoned job has had time to
    // land - the whole point of abandon() is that this result must never
    // get applied.
    EXPECT_TRUE(afterCommit.IsEqual(h.doc.getBody(bodyId)))
        << "the abandoned job's result must never land after commit";
}

TEST(MoveFaceAsyncPreview, SwitchingToLocalMidFlightIsNotOverwrittenByTheStaleGeneralResult) {
    // Round 1 finding 2; round 2 finding 5 (assert Local actually succeeded
    // instead of accepting either outcome).
    Harness h;
    TopoDS_Shape body = makeHolePlate(5);
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));
    GProp_GProps faceProps;
    BRepGProp::SurfaceProperties(face, faceProps);
    gp_Pnt centroid = faceProps.CentreOfMass();

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Rotate;
    mfc.st().moveFaceAngle = 0.05f;
    mfc.st().moveFacePivot = glm::vec3(
        static_cast<float>(centroid.X()), static_cast<float>(centroid.Y()), static_cast<float>(centroid.Z()));
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);

    mfc.updateMoveFace(ctx); // launches a general (whole-body loft) job
    ASSERT_TRUE(mfc.previewPending());

    // Mid-flight, the user ticks Local (only valid for a tilt, which this
    // is - localTweakApplies() requires FaceXform::Rotate and !isTwist).
    mfc.st().moveFaceLocal = true;
    mfc.updateMoveFace(ctx);
    ASSERT_EQ(mfc.st().moveFaceLocalRefusal, nullptr)
        << "Local must actually have succeeded for this test to prove anything";
    const TopoDS_Shape afterLocalSwitch = h.doc.getBody(bodyId);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline)
        mfc.pollPreview(ctx);
    ASSERT_FALSE(mfc.previewPending());

    // The STALE general job's result must not have landed on top of Local's.
    EXPECT_NEAR(volume(h.doc.getBody(bodyId)), volume(afterLocalSwitch), 1e-9)
        << "the general path's stale result landed after Local was ticked "
           "and overwrote it";
}

TEST(MoveFaceAsyncPreview, CancelAbandonsAnInFlightJobInsteadOfLeavingItAppliedLater) {
    // Mirrors the commit test above - cancel is a separate code path.
    Harness h;
    TopoDS_Shape body = makeHolePlate(5);
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Translate;
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);
    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.0f, 0.0f);

    mfc.updateMoveFace(ctx);
    ASSERT_TRUE(mfc.previewPending());
    mfc.cancelMoveFace(ctx); // restores the snapshot; must not block waiting for the job
    const TopoDS_Shape afterCancel = h.doc.getBody(bodyId);
    EXPECT_FALSE(mfc.previewPending());

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(afterCancel.IsEqual(h.doc.getBody(bodyId)))
        << "the abandoned job's result must never land after cancel";
}

TEST(MoveFaceAsyncPreview, ReturningToAPreviouslyAppliedValueRelaunchesRatherThanStayingOnTheSnapshot) {
    // A -> zero -> A. Round 2 finding 2's second half: since
    // launchMoveFacePreviewIfWanted no longer dedups against an "already
    // applied" cache (finding 1's fix), returning to A must relaunch and
    // land A again, not get silently skipped.
    Harness h;
    TopoDS_Shape body = makeHolePlate(5);
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Translate;
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);

    auto drainOnce = [&] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline)
            mfc.pollPreview(ctx);
        ASSERT_FALSE(mfc.previewPending());
    };

    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.0f, 0.0f); // A
    mfc.updateMoveFace(ctx);
    drainOnce();
    const double xAfterA = topFaceCentroidX(h.doc.getBody(bodyId));

    mfc.st().moveFaceVec = glm::vec3(0.0f, 0.0f, 0.0f); // zero: no-op, restores snapshot
    mfc.updateMoveFace(ctx);
    EXPECT_NEAR(topFaceCentroidX(h.doc.getBody(bodyId)), topFaceCentroidX(body), 1e-6);

    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.0f, 0.0f); // back to A
    mfc.updateMoveFace(ctx);
    drainOnce();

    EXPECT_NEAR(topFaceCentroidX(h.doc.getBody(bodyId)), xAfterA, 1e-6)
        << "returning to a previously-applied value must relaunch, not "
           "leave the body on the pristine snapshot";
}

// Round 3 finding 2 (second half): Task 1's tests hand-build MoveFaceOp
// configuration directly and never touch MoveFaceController::
// configureFaceOp() or currentMoveFaceKey() at all - so a desync between
// the two (the exact risk the "MUST stay in sync" comment on MoveFaceKey
// warns about) would pass every test in this plan except this one. This
// test drives a REAL Rotate gesture through controller state
// (moveFaceAngle/moveFaceRotAxis/moveFacePivot) so configureFaceOp's
// setRotationExplicit() branch and currentMoveFaceKey()'s faceRotTotal()
// branch both run for real, then verifies the landed result against a
// reference built from that SAME faceRotTotal() (not a hand-duplicated
// formula - that would just test the test).
TEST(MoveFaceAsyncPreview, ARealExplicitRotationGestureLandsTheCorrectGeometry) {
    Harness h;
    TopoDS_Shape body = makeHolePlate(5);
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = topFace(h.doc.getBody(bodyId));
    GProp_GProps faceProps;
    BRepGProp::SurfaceProperties(face, faceProps);
    gp_Pnt centroid = faceProps.CentreOfMass();

    MoveFaceController mfc;
    IopContext ctx = h.ctx();
    mfc.st().moveFaceActive = true;
    mfc.st().moveFaceBodyId = bodyId;
    mfc.st().moveFaceFace = face;
    mfc.st().moveFacePreviousShape = h.doc.getBody(bodyId);
    mfc.st().faceXformKind = FaceXform::Rotate;
    mfc.st().moveFaceIsTwist = false;
    mfc.st().moveFacePivot = glm::vec3(
        static_cast<float>(centroid.X()), static_cast<float>(centroid.Y()), static_cast<float>(centroid.Z()));
    mfc.st().moveFaceRotAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    mfc.st().moveFaceAngle = 0.04f; // a live ring drag, not yet baked into the accumulator
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);

    // Reference for angle=0.04 (A), built the same way configureFaceOp
    // would - computed BEFORE the gesture changes, so it does not depend
    // on mfc's later state.
    auto referenceFor = [&](float angle) {
        glm::mat3 R = rodrigues(glm::vec3(0.0f, 1.0f, 0.0f), angle);
        glm::vec3 pivot = mfc.st().moveFacePivot;
        glm::vec3 t = pivot - R * pivot;
        gp_Trsf trsf;
        trsf.SetValues(R[0][0], R[1][0], R[2][0], t.x,
                       R[0][1], R[1][1], R[2][1], t.y,
                       R[0][2], R[1][2], R[2][2], t.z);
        Document ref;
        int refId = ref.addBody(body, "plate");
        MoveFaceOp direct;
        direct.setBody(refId);
        direct.setFace(topFace(ref.getBody(refId)));
        direct.setKind(MoveFaceOp::Kind::Rotate);
        direct.setRotationExplicit(trsf);
        direct.setLoopMotion(true, std::vector<bool>(25, false), std::vector<bool>(25, false));
        EXPECT_TRUE(direct.execute(ref));
        return ref.getBody(refId);
    };
    const TopoDS_Shape refA = referenceFor(0.04f);
    const TopoDS_Shape refB = referenceFor(0.09f);
    // topFaceCentroidX is USELESS here: a Rotate pivots about the face's
    // own centroid (configureFaceOp), so rotating it never moves that
    // centroid, at any angle. Use topFaceZSpread (how far the now-tilted
    // top face's Z-extent spreads) instead - it scales with angle.
    const double zA = topFaceZSpread(refA);
    const double zB = topFaceZSpread(refB);
    ASSERT_GT(std::abs(zA - zB), 1e-3) << "0.04 and 0.09 rad must produce visibly different tilt";

    mfc.updateMoveFace(ctx); // launches angle=0.04 (A)
    ASSERT_TRUE(mfc.previewPending());
    // Round 3 finding 4 / round 5 finding 3: a test that submits only ONE
    // request and checks only the FINAL state would pass even with a
    // constant/broken MoveFaceKey - A landing incorrectly, then B landing
    // correctly afterward and overwriting it, is indistinguishable from
    // correct behaviour if you only look at the end. Submit B, then check
    // on EVERY poll from that point on that A's tilt never appears - once B
    // is submitted, currentMoveFaceKey() reflects B, so a discriminating
    // key must never let A's stale result land, at any point, not just
    // eventually.
    mfc.st().moveFaceAngle = 0.09f;
    mfc.updateMoveFace(ctx); // queues (or, if 0.04 already landed, launches) angle=0.09 (B)

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        const double z = topFaceZSpread(h.doc.getBody(bodyId));
        EXPECT_GT(std::abs(z - zA), 1e-3)
            << "A's stale result landed after B was submitted - the key does not discriminate";
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_FALSE(mfc.previewPending());

    EXPECT_NEAR(volume(h.doc.getBody(bodyId)), volume(refB), 1e-6);
    EXPECT_NEAR(topFaceZSpread(h.doc.getBody(bodyId)), zB, 1e-6);
}
```

Register in `tests/CMakeLists.txt` next to `test_move_face_preview`:

```cmake
add_executable(test_move_face_async test_move_face_async.cpp)
target_link_libraries(test_move_face_async PRIVATE materializr_core gtest gtest_main)
add_test(NAME test_move_face_async COMMAND test_move_face_async)
```

- [ ] **Step 2: Run to verify it fails**

Run: `cmake --build build --target test_move_face_async -j 8`
Expected: FAIL - `MoveFaceController` has no `st()`-driven async path yet; `updateMoveFace` still blocks inline, so `EXPECT_LT(callMs, 50.0)` fails once it compiles. If `IopContext`'s field order/types don't match what's declared in `InteractiveOpController.h` today, fix the struct literal to match - the field list in this plan was read from that header but a later edit to it takes priority.

- [ ] **Step 3: Add the dispatch members and overrides to `FaceOpControllers.h`**

In `class MoveFaceController`, add to the `public:` section (near the other lifecycle overrides):

```cpp
    void pollPreview(const IopContext& ctx) override;
    bool previewPending() const override;
```

Add to the `private:` section, alongside `MoveFaceState m_st;`:

```cpp
    // Off-thread preview for the general (Translate/Rotate/Scale/Twist)
    // path. Unlike PushPull/Shell this starts async on the FIRST call of a
    // gesture, not after measuring one slow inline frame: the cost here is
    // already known (movefaceop-freeze memory) to reach multiple seconds
    // well before "many holes" fixtures elsewhere in this project, so even
    // one inline hit is worth avoiding. At most ONE worker is ever
    // computing, checked via BOTH AsyncJob::running() (the tracked job)
    // AND AsyncJob::abandonedCount() (a parked job not yet finished -
    // round 4 finding 2: abandon() doesn't stop a thread, so a second
    // launch right after abandoning one would run concurrently with it).
    // m_mfPendingJob is a FULLY PREPARED job frozen at the moment of a real
    // trigger call, waiting for the worker to free up - never a bare flag
    // (round 4 finding 1: re-reading m_st at drain time would pick up
    // mid-drag drift the trigger never committed to).
    // beginMoveFace()/commitMoveFace()/cancelMoveFace() all reset every
    // field here.
    PreviewDispatch<MoveFaceKey> m_mfDispatch;
    AsyncJob<MoveFacePreviewResult> m_mfJob;
    std::unique_ptr<MoveFacePreviewJob> m_mfPendingJob;
    MoveFaceKey m_mfPendingKey;

    MoveFaceKey currentMoveFaceKey() const;
    void launchMoveFacePreviewIfWanted(const IopContext& ctx);
```

Add the two includes near the top of the file (alongside the existing `#include "AsyncJob.h"` if `InteractiveOpController.h` doesn't already transitively provide it - check first, it does provide `AsyncJob.h` per Task 1's reading, so only these two are new):

```cpp
#include "MoveFaceDispatch.h"
#include "MoveFacePreview.h"
#include "PreviewDispatch.h"
```

(`PreviewDispatch.h` is also already included transitively via `InteractiveOpController.h`'s own include of it for the base class's `m_dispatch` - verify with `grep -n PreviewDispatch src/app/InteractiveOpController.h` before adding a duplicate; if already visible, skip re-including it.)

- [ ] **Step 4: Implement `currentMoveFaceKey()` in `FaceOpControllers.cpp`**

Place it directly above `configureFaceOp` (they must be kept in sync - see the comment already written into `MoveFaceDispatch.h` in Task 1):

```cpp
MoveFaceKey MoveFaceController::currentMoveFaceKey() const {
    MoveFaceKey k;
    k.bodyId = m_st.moveFaceBodyId;
    k.kind = m_st.faceXformKind;
    k.isTwist = m_st.moveFaceIsTwist;
    k.moveVec = m_st.moveFaceVec;
    k.pivot = m_st.moveFacePivot;
    if (m_st.faceXformKind == FaceXform::Rotate && !m_st.moveFaceIsTwist)
        k.rotMat = faceRotTotal();
    k.twistAngle = m_st.moveFaceTwist;
    k.scaleUniform = m_st.moveFaceScaleUniform;
    k.scaleFactor = m_st.moveFaceScale;
    k.scaleA = m_st.moveFaceScaleA;
    k.scaleB = m_st.moveFaceScaleB;
    k.scaleAxisA = m_st.moveFaceAxisA;
    k.scaleAxisB = m_st.moveFaceAxisB;
    return k;
}
```

- [ ] **Step 5: Implement `launchMoveFacePreviewIfWanted`, `pollPreview`, `previewPending`**

**Round 4 correction (findings 1-3) - supersedes round 3's fix.** Round 3 correctly identified that launching unconditionally causes unbounded concurrent rebuilds, and bounded to "one worker plus a pending-retry FLAG." Two things wrong with that: (a) the flag remembered only THAT a request arrived, not WHAT it was - by the time it's drained, `launchMoveFacePreviewIfWanted` re-reads LIVE `m_st`, which the viewport drag can have moved on from since (round 1's finding 5, reintroduced); (b) `updateMoveFace`'s zero/Local branches called `m_mfJob.abandon()`, which does not stop the thread - it only makes `AsyncJob::running()` report false immediately while the abandoned thread keeps computing, so a NEW launch right after would run concurrently with it, defeating the one-worker bound entirely; and separately, `pollPreview`'s `if (!result) return;` skipped checking the pending flag on every poll where `take()` had nothing (which, after an abandon(), is every poll from then on, since an abandoned job's result is never surfaced by `take()` at all) - so the flag could never drain, `previewPending()` could get stuck true forever, and a genuinely queued request could be silently lost.

The corrected design: at most one worker thread ever computing, verified by checking BOTH `AsyncJob::running()` (the tracked current job) AND `AsyncJob::abandonedCount()` (parked jobs not yet finished) after a `reap()` - `abandon()` is still used at gesture-transition points (it is still the right way to say "never apply this job's result"), but nothing launches a new job while `abandonedCount() > 0` either. The retry mechanism stores a FULLY PREPARED job (`MoveFacePreviewJob::prepare()` already runs synchronously and cheaply - a `BRepBuilderAPI_Copy` of one body, no boolean/loft yet - so capturing "what to run next" this way freezes the exact configuration at the moment of the real trigger call, immune to any later drift in `m_st`) rather than a bare flag, and `pollPreview` checks it unconditionally, never behind an `if (!result) return`. `MoveFaceKey` also gained a `bodyId` field (see its definition above) as a belt-and-suspenders guard against a job from one gesture ever landing on a different gesture's (possibly different) body.

`m_mfPendingJob`/`m_mfPendingKey` were already added to the member block in Step 3.

```cpp
void MoveFaceController::launchMoveFacePreviewIfWanted(const IopContext& ctx) {
    if (!faceXformNontrivial()) { m_mfPendingJob.reset(); return; } // nothing to preview
    m_mfDispatch.inlinePreviewTook(PreviewDispatch<MoveFaceKey>::kAsyncPreviewMs);
    const MoveFaceKey want = currentMoveFaceKey();
    // Prepare NOW, from live m_st, while this call is itself the real
    // trigger - this is what makes it safe to launch this exact job LATER
    // without re-reading m_st at that point (round 4 finding 1).
    std::unique_ptr<MoveFacePreviewJob> job = MoveFacePreviewJob::prepare(
        m_st.moveFacePreviousShape, m_st.moveFaceFace,
        [this](MoveFaceOp& op) { configureFaceOp(op); });
    if (!job) { m_mfPendingJob.reset(); return; } // last landed shape (or the pristine snapshot) stays on screen
    m_mfJob.reap();
    if (m_mfJob.running() || m_mfJob.abandonedCount() > 0) {
        // Something is still genuinely computing, tracked or merely parked
        // (round 4 finding 2: abandon() doesn't stop a thread, so a job
        // this controller no longer wants can still be occupying a core).
        // Freeze this fully-configured job to run once everything clears,
        // replacing whatever was queued before it.
        m_mfPendingJob = std::move(job);
        m_mfPendingKey = want;
        return;
    }
    // Round 5 finding 1: this call is launching THIS request right now,
    // which makes any OLDER frozen request in m_mfPendingJob obsolete -
    // clear it, or a stale queued job could fire later and both waste a
    // multi-second rebuild nobody wants and delay the NEXT real request
    // behind it.
    m_mfPendingJob.reset();
    std::shared_ptr<MoveFacePreviewJob> shared = std::move(job);
    if (m_mfJob.launch([shared] { return shared->run(); }))
        m_mfDispatch.launched(want);
}

void MoveFaceController::pollPreview(const IopContext& ctx) {
    m_mfJob.reap();
    std::optional<MoveFacePreviewResult> result = m_mfJob.take();
    if (result) {
        // A result is UNWANTED - discard it, nothing to apply - once the
        // gesture ended, switched to hole-move, switched to the Local
        // rebuild, or returned to a no-op value while this job was in
        // flight. Local's checkbox is deliberately NOT part of MoveFaceKey
        // (see the comment on MoveFaceKey), so this check is what actually
        // protects a landed local rebuild from a stale result overwriting
        // it.
        const bool wanted = m_st.moveFaceActive && !m_st.moveHoleMode &&
                            !localTweakApplies() && faceXformNontrivial();
        if (wanted) {
            const MoveFaceKey now = currentMoveFaceKey();
            if (m_mfDispatch.finished(now) && result->ok && !result->shape.IsNull()) {
                ctx.doc.updateBody(m_st.moveFaceBodyId, result->shape);
                ctx.markMeshesDirty();
            }
            // A refused result, or a stale one (key mismatch - a newer
            // request queued behind this one), leaves whatever is
            // currently on the document.
        }
    }
    // Round 4 finding 3: this runs UNCONDITIONALLY, never behind
    // `if (!result) return` - an abandoned job's result never reaches
    // take() at all, so gating this on `result` being present could leave
    // a queued job (and previewPending()) stuck forever once anything gets
    // abandoned.
    if (m_mfPendingJob) {
        const bool stillWanted = m_st.moveFaceActive && !m_st.moveHoleMode &&
                                 !localTweakApplies() && faceXformNontrivial();
        if (!stillWanted) {
            m_mfPendingJob.reset(); // the gesture moved on before this ever ran - drop it, not launch it
        } else {
            m_mfJob.reap();
            if (!m_mfJob.running() && m_mfJob.abandonedCount() == 0) {
                std::shared_ptr<MoveFacePreviewJob> shared = std::move(m_mfPendingJob);
                const MoveFaceKey key = m_mfPendingKey;
                if (m_mfJob.launch([shared] { return shared->run(); }))
                    m_mfDispatch.launched(key);
            }
        }
    }
}

bool MoveFaceController::previewPending() const {
    // Round 5 finding 2: does NOT count AsyncJob::abandonedCount() (round
    // 4's version did, to keep the render loop polling until an abandoned
    // job actually finishes). That was unnecessary: per
    // PushPullController::previewPending()'s own comment, the app's main
    // loop already polls every controller once per iteration at an IDLE
    // floor rate regardless of previewPending() - abandoned jobs are reaped
    // there for free, at idle cost, exactly as Push/Pull already relies on.
    // Forcing FULL-rate rendering (what previewPending()==true actually
    // buys) for the remaining duration of a job nobody wants anymore, after
    // every commit/cancel, would waste real rendering resources and
    // compete with the abandoned worker for CPU for no benefit. This only
    // reports true for work the controller still WANTS an answer for: a
    // tracked job, or one frozen and waiting to launch.
    return m_mfJob.running() || static_cast<bool>(m_mfPendingJob);
}
```

- [ ] **Step 6: Rewrite the general branch of `updateMoveFace`**

**CORRECTION (round 1, finding 3):** the ORIGINAL code's very first line unconditionally restores `m_st.moveFacePreviousShape` onto the document before doing anything else - a leftover from the old inline design, where every call recomputed from scratch and the restore undid the PREVIOUS preview frame first. `MoveFacePreviewJob::prepare()` already takes `m_st.moveFacePreviousShape` as its own base (never reads `ctx.doc`), so that restore is no longer needed for correctness - and keeping it unconditionally is actively wrong: it would erase an already-landed async result the instant the NEXT event fires (a second stepper click, say), and because `PreviewDispatch::shouldLaunch()` refuses to relaunch a key that is already `m_applied`, nothing would ever put the shape back (a value the user previously dialled in and dialled back to would get silently wiped to the pristine original). The restore is now needed ONLY when the gesture returns to a true no-op (`!faceXformNontrivial()`), where it must also tell the dispatch the applied preview came off the body (`retracted()`, mirroring `PushPullController::updatePushPull`'s identical zero-distance case) so returning to a non-zero value later asks again instead of matching a stale `m_applied` key.

Replace `updateMoveFace`'s general section - from the `// Snap an in-plane face SLIDE...` comment (today's `FaceOpControllers.cpp:1610`) through the end of the function (today's `:1653`) - with:

```cpp
    // Snap an in-plane face SLIDE to the grid step (issue #24): unchanged.
    if (m_st.faceXformKind == FaceXform::Translate && ctx.snapToGrid &&
        ctx.gridStep > 0.0f) {
        const float step = ctx.gridStep;
        const float a = std::round(glm::dot(m_st.moveFaceVec, m_st.moveFaceAxisA) / step) * step;
        const float b = std::round(glm::dot(m_st.moveFaceVec, m_st.moveFaceAxisB) / step) * step;
        m_st.moveFaceVec = a * m_st.moveFaceAxisA + b * m_st.moveFaceAxisB;
    }

    if (!faceXformNontrivial()) {
        // Back to a no-op: take any landed preview off the body and tell the
        // dispatch so a later non-zero value is asked for again rather than
        // matching a stale "already applied" key.
        m_mfJob.abandon();
        m_mfDispatch.retracted();
        m_mfPendingJob.reset();
        ctx.doc.updateBody(m_st.moveFaceBodyId, m_st.moveFacePreviousShape);
        ctx.markMeshesDirty();
        moveFaceSlideSketches(ctx, glm::vec3(0.0f));
        return;
    }
    if (localTweakApplies()) {
        // Local rebuild bypasses the async path entirely (cheap, and must
        // win over any in-flight general-path job - see pollPreview).
        //
        // ROUND 2 CORRECTION (finding 3): applyLocalTweak() resolves
        // m_st.moveFaceFace against whatever is CURRENTLY on ctx.doc, and
        // FaceTweak::moveFace() throws FaceNotFound when that face isn't a
        // live sub-shape of the current body. If a general-path async result
        // landed earlier in this gesture (or Local was on, off, then back
        // on), the live body is no longer the pristine snapshot the original
        // face came from - restore it FIRST, unconditionally, right here
        // (this is the one place in the general branch that still needs an
        // explicit restore; the async branch below deliberately does not).
        m_mfJob.abandon();
        m_mfPendingJob.reset();
        ctx.doc.updateBody(m_st.moveFaceBodyId, m_st.moveFacePreviousShape);
        if (!applyLocalTweak(ctx))
            ctx.doc.updateBody(m_st.moveFaceBodyId, m_st.moveFacePreviousShape);
        ctx.markMeshesDirty();
        return;
    }
    if (m_st.faceXformKind == FaceXform::Translate) moveFaceSlideSketches(ctx, m_st.moveFaceVec);
    launchMoveFacePreviewIfWanted(ctx);
```

Note what is DELETED relative to the original: the unconditional `ctx.doc.updateBody(m_st.moveFaceBodyId, m_st.moveFacePreviousShape); ctx.markMeshesDirty();` pair that used to open the function. The document now shows whatever was last landed (or the pristine snapshot, if nothing has landed yet in this gesture) until a fresh worker result arrives - this is the intended "trails the input" behaviour, not a regression.

- [ ] **Step 7: Reset dispatch/job state at every lifecycle boundary**

Not just gesture START: an in-flight job left completely untouched past commit/cancel would keep landing results onto a gesture that has already ended (or, worse, onto whatever the NEXT gesture's `m_st.moveFaceBodyId` happens to be, though `MoveFaceKey::bodyId` now guards against that specifically). Abandon at all three boundaries so the controller stops caring about that job's result. This does NOT make `previewPending()` go false immediately (round 4 finding 2/3 - `previewPending()` correctly stays true for as long as `AsyncJob::abandonedCount() > 0`, i.e. for as long as the abandoned thread is genuinely still computing) - that's the app's render loop correctly reflecting real, ongoing CPU work, not a bug to hide.

In `beginMoveFace`, add near the other `m_st.*` resets (right after `m_st.moveHoleWall.Nullify();`):

```cpp
    m_mfJob.abandon(); // a job from the PREVIOUS gesture must never land into this one
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
```

In `commitMoveFace`, add as the very first lines (before the hole-move-mode branch):

```cpp
    m_mfJob.abandon();
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
```

In `cancelMoveFace`, add as the very first lines:

```cpp
    m_mfJob.abandon();
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
```

- [ ] **Step 8: Build and run the new tests**

Run: `cmake --build build --target test_move_face_async -j 8 && ./build/tests/test_move_face_async`
Expected: PASS, both cases.

- [ ] **Step 9: Run the full suite**

Run (unsandboxed): `cd build && ctest --output-on-failure`
Expected: every existing test passes, plus both new test binaries. Pay particular attention to any existing Move Face test (`test_moveface_hollow`, `test_twist_face` if it exists) - they call `MoveFaceOp::execute()` directly, not through the controller, so they should be unaffected, but confirm.

- [ ] **Step 10: Commit**

```bash
git add src/app/FaceOpControllers.h src/app/FaceOpControllers.cpp tests/test_move_face_async.cpp tests/CMakeLists.txt
git commit -m "app: Run Move Face's body rebuild off the main thread"
```

---

### Task 3: In-app verification, changelog, final regression

**Files:**
- Modify: `docs/changelog.md` (add an `[Unreleased]` / `Fixed` entry)
- No source changes.

- [ ] **Step 1: Reproduce the original bug's fixture in the running app**

Use the `run-materializr` skill to build and launch the app. Create (or script via a fixture generator, matching the recipe in `load-progress-vsync` memory for reaching real timing) a body with at least 100 holes on one face - reuse the same grid pattern as `makeHolePlate()` above. Select that face, invoke Move Face (Translate), drag it, and release.

Expected BEFORE this plan (do this once, on a build from before Task 2's commit, to confirm the repro): the app appears to hang / beachball for roughly 1 second (100 holes, per the perf table) after mouse-up.

Expected AFTER Task 2: mouse-up returns control immediately; the body visibly updates a beat later (worker latency) without the window becoming unresponsive. Take a screenshot of the result for the PR/commit record if the skill supports it.

- [ ] **Step 2: Repeat with the panel controls**

With the same body, use the Tilt stepper (`+10`, `+1`) and the numeric Tilt field (type a multi-digit value) to confirm neither hangs the window. Toggle "Local" on/off once to confirm that path (unaffected by this plan, still routes through `FaceTweakOp`) still works.

- [ ] **Step 3: Commit the panel's real op still matches after Confirm**

Commit the gesture (the panel's Confirm button / Enter) and confirm the resulting body's geometry matches what the async preview showed (no visible "jump" between the last preview frame and the committed result) - `commitMoveFace` is unmodified by this plan and re-runs `MoveFaceOp::execute()` synchronously on the real document, so this is a regression check on Task 2's landing logic, not new behavior.

- [ ] **Step 4: Update the changelog**

In `docs/changelog.md`, under `## [Unreleased]` / `### Fixed`, add an entry in the house style (see the existing Push/Pull entry for the pattern: state the user-visible symptom, the measured numbers, and the fix):

```markdown
- **Move Face no longer freezes on a many-hole face.** Translating, tilting,
  twisting, or scaling a face with many holes in it (a perforated plate, a
  vented panel) ran the whole body rebuild on the main thread once per
  mouse-release, stepper click, or keystroke - up to 15 seconds on a
  400-hole face (measured; the cost grows faster than linear with hole
  count, since each hole gets its own loft and boolean cut against a
  progressively more complex body). The rebuild now runs on a worker thread
  the same way Push/Pull's and Shell's already do; the body updates a beat
  after you release instead of freezing the window. Dragging the face
  itself was already cheap (only a ghost silhouette moves mid-drag) and is
  unaffected.
```

- [ ] **Step 5: Full regression + validate-code**

Run (unsandboxed): `cd build && ctest --output-on-failure`
Then invoke the `validate-code` skill (or the project's Stop-hook gate will require it anyway) for the final security/performance/regression pass over the whole diff across all three tasks.

- [ ] **Step 6: Commit**

```bash
git add docs/changelog.md
git commit -m "docs: Note the Move Face async-preview fix in the changelog"
```

---

## Self-Review Notes

- **Spec coverage:** every claim in `movefaceop-freeze` memory is addressed - the O(n^1.6) cost (Task 1/2, moved off-thread), the four trigger sites (release deferred-rebuild, stepper click, numeric field keystroke, Local checkbox toggle - all funnel through the single `updateMoveFace()` general branch rewritten in Task 2), the stale-dispatch-key risk from `serializeParams()` (avoided by hand-building `MoveFaceKey`, called out explicitly in both the memory and this plan), and manual in-app confirmation (Task 3).
- **Explicitly out of scope, and why:** hole-move mode and local-tweak mode were checked during investigation and do not call `MoveFaceOp::execute()` at all (they use `MoveHoleOp` and `FaceTweakOp` respectively, neither shown to be expensive) - Task 2's `pollPreview` discards a landed result while either is active, for exactly this reason.
- **Known risk carried into implementation:** `currentMoveFaceKey()` duplicates the field list `configureFaceOp()` reads. This is the same tradeoff `PushPullKey{distance, symmetric}` already makes in this codebase, but it is a real desync risk if a future change adds a new configurable parameter to Move Face without updating the key.

**Round 1 Codex review corrections (all incorporated, see inline `CORRECTION` comments in Task 2 for exactly where):**
1. `launchMoveFacePreviewIfWanted` never actually forced async mode - `PreviewDispatch` defaults to inline and `reset()` re-disables it; fixed by calling `inlinePreviewTook(kAsyncPreviewMs)` on every launch attempt.
2. A stale general-path result landing after the user switched to Local mid-gesture would have overwritten the (correct) local rebuild, since `moveFaceLocal` isn't part of `MoveFaceKey`. Fixed in `pollPreview` by discarding any result while `localTweakApplies()` is true, rather than growing the key.
3. The unconditional restore-to-snapshot at the top of the original `updateMoveFace` would erase an already-landed preview on every subsequent event and could never be un-erased (a value the user returns to would match `PreviewDispatch`'s "already applied" cache and never relaunch). Fixed by deleting the unconditional restore and only restoring (with an explicit `retracted()`) when the gesture returns to a true no-op.
4. `pollPreview` early-returned before calling `take()` whenever the gesture was inactive, so a job still running past commit/cancel could never be reaped and `previewPending()` would stay true indefinitely (keeping the app's render loop spinning for the job's full multi-second cost). Fixed by making reap/take unconditional and adding explicit `abandon()` calls at all three lifecycle boundaries (begin/commit/cancel).
5. `pollPreview` auto-relaunching on a stale key would have read still-changing, not-yet-committed drag state (the viewport drag defers `updateMoveFace()` to release) and started an unrequested rebuild mid-drag, bypassing grid-snap and sketch-follow. Fixed by discarding stale results instead of relaunching from inside `pollPreview` - only the four real trigger sites ever launch.
6. The plan only added `MoveFacePreview.cpp` to the test-only `materializr_core` target; the app itself has its own separate, explicit source list (confirmed at `CMakeLists.txt:205`) and would fail to link. Fixed by adding it to both, and adding `FaceOpControllers.cpp` to `materializr_core` too (needed for Task 2's controller test, and not previously compiled into it).
7. Test gaps: a missing `#include "modeling/MoveFaceOp.h"`, volume-only assertions that can't distinguish a stale result from a correct one on a volume-preserving Translate, no assertion that a worker actually launched, a machine-dependent fixed timing threshold, and no coverage of the commit/cancel-abandons-a-job or Local-mid-flight cases. All fixed: added `topFaceCentroidX`, a same-run measured relative timing bound, explicit `previewPending()` assertions, and two new tests.

**Round 2 Codex review corrections (all incorporated):**
1. **The round-1 fix for finding 5 combined with `shouldLaunch()`'s busy-gate to silently drop legitimate requests**: submit A, submit B before A lands, and B would never launch (refused as "busy") nor get retried (pollPreview no longer auto-relaunches, by round 1's own fix). Root cause: `shouldLaunch()`'s `m_running` gate exists for Push/Pull's per-frame `update()`, which needs "wait for the current job, then decide" so it doesn't spawn 60 threads/second - Move Face's four trigger sites are discrete events with no such flood to guard against. Fixed by dropping `shouldLaunch()` entirely and always launching immediately; `AsyncJob::launch()`'s own built-in abandon-and-replace makes this safe.
2. Falls out of the finding-1 fix: with no busy-gate and no "already applied" dedup, a zero/Local transition needs no unwedging (nothing was wedged) and "general A → Local → general A" correctly relaunches A instead of trusting a stale cache.
3. Deleting the unconditional snapshot-restore (round 1's fix for finding 3) broke `applyLocalTweak()`, which resolves `m_st.moveFaceFace` against whatever is currently on `ctx.doc` and throws `FaceNotFound` if a landed general-path result left the body somewhere the original face isn't a live sub-shape of. Fixed by restoring the snapshot explicitly, unconditionally, as the first thing inside the `localTweakApplies()` branch specifically (the async branch still does not restore).
4. `materializr_core` doesn't compile `CylindricalPick.cpp`, a hard link dependency of `FaceOpControllers.cpp` (`detectCylindricalPick()` at `:906`) that round 1's fix for finding 6 missed. Fixed by adding it alongside `FaceOpControllers.cpp`.
5. Test gaps: the Local test accepted either success or failure from `applyLocalTweak`, proving nothing; no cancel test existed (only commit); no A→zero→A test existed. Fixed: the Local test now sets a correct pivot and asserts `moveFaceLocalRefusal == nullptr`; added `CancelAbandonsAnInFlightJob...` (mirrors the commit test) and `ReturningToAPreviouslyAppliedValueRelaunches...`. Explicit-rotation/nonuniform-scale/Twist coverage is deliberately left to Task 1's `MoveFacePreviewJob` tests (which already cover all four kinds at the job level) rather than duplicated at the controller/dispatch level, which doesn't care which kind is active - this is a scope decision, not an oversight.
6. Global Constraints literally forbade touching `commitMoveFace()`/`cancelMoveFace()` while Step 7 required both. Fixed by narrowing the constraint to "don't change their modeling behavior" and calling out the two-line async-bookkeeping exception explicitly.

**Round 3 Codex review corrections:**
1. Round 2's own fix for round 1's finding 1 was itself wrong: dropping `shouldLaunch()` entirely and always calling `m_mfJob.launch()` immediately means every rapid event during one still-running multi-second computation spawns ANOTHER concurrent OCCT rebuild (`AsyncJob::abandon()` parks a thread, it does not cancel it - the abandoned computation keeps consuming CPU and holding its own scratch-Document copy until it finishes on its own). Fixed by bounding to at most one worker in flight, checked via `AsyncJob::running()` directly, plus a single `m_mfPendingRelaunch` flag capturing "a real request arrived while busy" - set only from `launchMoveFacePreviewIfWanted` (a real trigger), never inferred from `pollPreview` reading ambient state, so round 1's finding 5 stays fixed too. This also still resolves round 1's finding 2 in full (nothing to wedge: the pending flag is drained deterministically once the single worker frees up).
2. The round-2 claim that Task 1 already covered "all four kinds" was false - only `setRotation()` (not `setRotationExplicit()`, what `configureFaceOp` actually calls) and uniform `setScaleFactor()` were exercised; `setTwist()` was never called at all. Fixed by adding `ExplicitRotationNonUniformScaleAndTwistAlsoMatchADirectExecute` to Task 1. Separately, and more importantly: every Task 1 test hand-builds `MoveFaceOp` configuration directly, so a desync between `MoveFaceController::configureFaceOp()` and `currentMoveFaceKey()` - the exact risk flagged as a "known risk" in this very section - could pass every test in the plan. Fixed by adding `ARealExplicitRotationGestureLandsTheCorrectGeometry` to Task 2, which drives a real gesture through controller state and checks the result against a reference built from the controller's own `faceRotTotal()`.
3. Test gaps: no assertion that the design is actually bounded to one worker; the Local test didn't switch back to general afterward; the A→zero→A test didn't stress abandonment specifically; commit/cancel tests didn't verify document state after completion. Accepted the boundedness point as now true BY CONSTRUCTION (finding 1's fix makes a second concurrent launch structurally impossible - `launchMoveFacePreviewIfWanted` checks `running()` before ever calling `m_mfJob.launch()`), and did not add a private-state-poking counter to assert it redundantly. Declined the request for "controllable worker barriers" / deterministic synchronization primitives inside `MoveFacePreviewJob` purely for test purposes: no other async-preview code in this codebase (`test_pushpull_preview.cpp` included) uses such a seam, bounded-deadline polling is the established pattern throughout, and adding test-only hooks to production code isn't justified by what's actually being verified here (key-matching and landing logic, not thread-scheduling fairness). Logged as a deliberate scope boundary. **This "by construction" claim about boundedness turned out to be WRONG - see round 4.**

**Round 4 Codex review corrections:** round 3's own fix was itself broken - `AsyncJob::abandon()` does not stop a thread, it only stops tracking it, so `updateMoveFace`'s zero/Local branches calling `m_mfJob.abandon()` made `running()` report false immediately while the abandoned thread kept computing; a launch right after would run concurrently with it, exactly the pileup round 3 was supposed to prevent. Separately, storing only a bare "please retry" flag (not the actual configuration) meant the eventual retry re-read live `m_st`, which the deferred viewport drag can mutate without ever calling `updateMoveFace()` - reintroducing round 1's finding 5. And `pollPreview`'s `if (!result) return` skipped checking that flag on every poll after an abandon (an abandoned job's result never reaches `take()` at all), so the flag - and `previewPending()` - could get stuck forever.

Fixed all three together: launches are now gated on BOTH `AsyncJob::running()` and `AsyncJob::abandonedCount()` (after `reap()`), so a second launch is refused for as long as ANY thread - tracked or merely parked - is still genuinely computing; the retry mechanism stores a FULLY PREPARED `MoveFacePreviewJob` (built from live `m_st` at the moment of the real trigger call, immune to later drift) rather than a bare flag; and the pending-job check in `pollPreview` runs unconditionally, never behind `if (!result) return`. `previewPending()` now also reports true while `abandonedCount() > 0`, correctly reflecting that real CPU work is still happening rather than lying that the controller is idle - this matches `InteractiveOpController::pollPreview`'s own documented contract that abandoned jobs are polled for "whether or not a gesture is active." Also added `MoveFaceKey::bodyId` as a cross-gesture safety net, fixed the commit/cancel tests' timing expectations (previewPending() now correctly takes as long as the real computation does, not ~200ms) and added document-integrity assertions to both, and added a key-discrimination step to the real-rotation controller test (submit two different angles before either lands, so a constant/broken key couldn't pass by accident). **The `previewPending()` claim above ("also true while abandonedCount() > 0") turned out to be an over-correction - see round 5.**

**Round 5 Codex review corrections (design confirmed sound overall; three narrower issues found):**
1. `launchMoveFacePreviewIfWanted`'s IMMEDIATE-launch branch (worker free, launching right now) never cleared a PRE-EXISTING `m_mfPendingJob` left over from an earlier call. Scenario: A abandoned-but-still-finishing, B queued as pending; before A finishes, a THIRD event C arrives when A has meanwhile been reaped elsewhere - C launches immediately without touching B, so B (now stale) fires later anyway, wasting a rebuild and delaying whatever comes after it. Fixed with one line: `m_mfPendingJob.reset();` right before an immediate launch, since launching C makes any older queued request obsolete by definition.
2. `previewPending()` counting `AsyncJob::abandonedCount() > 0` (round 4's fix for the "stuck forever" bug) was itself an over-correction: per `PushPullController::previewPending()`'s own comment, the app's main loop already polls every controller once per iteration at an idle floor rate regardless of `previewPending()`, so an abandoned job gets reaped there for free without needing full-rate rendering forced for its whole remaining duration. Reverted `previewPending()` to `running() || pendingJob` (no `abandonedCount()`), matching the established Push/Pull precedent exactly. This also meant reverting the commit/cancel tests' timing expectations back toward "goes false promptly" (now correct again, since abandon() alone does make it false immediately) while KEEPING the document-integrity assertions added in round 4 (poll unconditionally for a fixed duration afterward, independent of previewPending(), to prove the abandoned result never lands).
3. The key-discrimination test (added in round 3, refined in round 4) checked only the FINAL landed state, so a broken/constant `MoveFaceKey` that let stale A land first and correct B land second and overwrite it would pass undetected. Fixed by asserting INSIDE the polling loop, on every iteration after B is submitted, that A's result never appears on the document - not just checking the end state. Also discovered while building this fix that `topFaceCentroidX` cannot distinguish two different Rotate angles at all (a rotation pivots about the face's own centroid, which by definition does not move under rotation about itself) - added `topFaceZSpread` (the tilted face's Z-extent, which does scale with angle) as the correct metric for the Rotate-specific tests, and reused `topFaceCentroidX` only where it's actually valid (Translate).
