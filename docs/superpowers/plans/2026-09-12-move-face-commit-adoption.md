# Move Face Commit Adoption Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Move Face tool's final Confirm click adopt the off-thread preview's already-computed result instead of recomputing it, closing the one freeze `docs/superpowers/plans/2026-09-10-move-face-async-preview.md` deliberately left open (its line 15: "do not change the modeling behavior of `commitMoveFace()`").

**Architecture:** `MoveFaceOp` gains the generic `Operation::Precomputed`/`canAdopt` adoption mechanism `ShellOp`/`TaperOp`/`ScaleFaceOp` already use - a `previewKey()` override plus an early adopt-check inside `execute()` that skips the expensive per-hole loft+cut work. `MoveFaceController` (which does not derive from the shared `InteractiveOpController` preview-landing machinery those three ride on) gets its own small landed-result cache, populated when a preview lands and offered to the commit op right before it's pushed onto `History`.

**Tech Stack:** C++17, OpenCASCADE (OCCT), existing `Operation`/`Document`/`History` core, existing `AsyncJob`/`PreviewDispatch` off-thread infra (unchanged by this plan - only `MoveFacePreview.h/.cpp` and `MoveFaceController` gain new fields).

**Spec:** No separate spec doc - this is a bounded follow-up to an already-shipped, already-reviewed feature. The originating gap is `docs/superpowers/plans/2026-09-10-move-face-async-preview.md` (line 15's explicit scope-out) plus the project memory at `movefaceop-freeze.md` (FIXED section, "RESIDUAL" paragraph). Read both before starting; they explain WHY `commitMoveFace()` still runs synchronously today and WHY that was the right call at the time.

## Global Constraints

- C++17, this repo's existing OCCT version (see `CMakeLists.txt` - no version bump needed, no new OCCT headers required).
- No new third-party dependency.
- Do not change `buildFeature`/`buildTwistFeature`'s modeling algorithm. This plan only changes WHEN they run, never what they compute.
- Do not touch `MoveHoleOp`'s commit path (the `moveHoleMode` branch in `commitMoveFace`, `FaceOpControllers.cpp` lines ~1780-1798) - Move Hole has its own single-boolean cost profile (never benchmarked as a freeze) and is out of scope.
- Do not implement `wantsDeferredCommit()` for Move Face - that mechanism moves a recompute onto a cancellable progress window, it does not avoid the recompute. Adoption is strictly better when it fires; the rare fallback (preview never landed, or params changed after the last landed preview) is bounded by today's cost, and fires strictly LESS often than today (see "Declined findings" below for the precise claim).
- Do not implement cooperative cancellation for `AsyncJob`/`m_mfJob.abandon()` - see "Declined findings" below.
- Do not add a validity re-check (`BRepCheck_Analyzer`, solid/shell-count, volume-ratio) to the adopt branch - see "Declined findings" below.
- Follow this repo's existing `%a` hex-float convention for any preview key encoding numeric state (see `TaperOp::previewKey`, `ShellOp::previewKey`) - never `%f`/`%g`, which round and can falsely equate two different values.
- This plan goes through `claudex-loop:codex-review` (2 rounds) before any of its code is written, per this project's CLAUDE.md hard rule (the concurrency/adoption-safety trap is exactly what that rule exists for).

---

## Task 1: `MoveFaceOp` gains `previewKey()`, an adopt branch, and a shared `applyResult()` tail

**Files:**
- Modify: `src/modeling/MoveFaceOp.h`
- Modify: `src/modeling/MoveFaceOp.cpp`
- Test: `tests/test_preview_adoption.cpp`

**Interfaces:**
- Consumes: `Operation::Precomputed`, `Operation::setPrecomputedResult(TopoDS_Shape base, TopoDS_Shape result, std::string selectionKey)`, `Operation::takePrecomputed()`, `Operation::canAdopt(const Precomputed&, const TopoDS_Shape& liveBody, const std::string& liveKey)` - all already defined at `src/core/Operation.h:163-219`, unchanged by this task.
- Produces: `MoveFaceOp::previewKey(const TopoDS_Shape& base) const` (public, overrides the base class's default-returns-empty version at `Operation.h:192`) - Task 2 and Task 3 both call this indirectly (Task 2's worker calls it directly; Task 3's `canAdopt` check inside `execute()` calls it via the existing mechanism, no new caller needed there).

### Step 1: Add the `previewKey()` declaration to the header

In `src/modeling/MoveFaceOp.h`, add this public method declaration right after `bool rehydrateFromReload(...)` (currently the last method before the `private:` section, line 95):

```cpp
    std::string previewKey(const TopoDS_Shape& base) const override;
```

Also add one new private method declaration, right after `void setLoopMotion(...)` is irrelevant here - add it in the `private:` section, after the existing `materializr::topo::Ref m_faceRef;` field (the last line before the closing brace, currently line 125):

```cpp
    // Shared tail for both the adopt and full-recompute paths: writes the
    // result, records the transform for undo, and slides on-face sketches.
    void applyResult(Document& doc, const TopoDS_Shape& result, const gp_Trsf& topT);
```

Also add, in the `public:` section (anywhere, e.g. right after `getPreviousShape()`):

```cpp
    // Diagnostic + test hook: how many times an offered Precomputed candidate
    // was actually adopted vs how many times execute() ran the full
    // recompute (offered-but-rejected counts as a recompute). Process-wide,
    // never reset automatically - a caller that needs a delta captures the
    // counts before and after. Not gated on isVerbose(): the counters
    // themselves cost nothing at rest; only the stderr line in Step 6 below
    // is gated.
    static int adoptedCount();
    static int recomputedCount();
```

### Step 2: Write the failing tests for adoption (before touching `execute()`)

Add this block to `tests/test_preview_adoption.cpp`, right after the existing `TEST(PreviewAdoption, ScaleFaceAdoptsAndRejectsOnTheSameTerms)` test (find it by searching the file; append after its closing `}`). Add the include at the top of the file alongside the other op includes:

```cpp
#include "modeling/MoveFaceOp.h"
```

Then the tests:

```cpp
TEST(PreviewAdoption, MoveFaceAdoptsAndRejectsOnTheSameTerms) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto mk = [&] {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(id);
        op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Translate);
        op->setMoveVector(gp_Vec(2.0, 0.0, 0.0));
        return op;
    };

    {   // Adopts when the body and selection still match.
        auto op = mk();
        op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));
        ASSERT_TRUE(op->execute(doc));
        EXPECT_TRUE(isMarker(doc.getBody(id))) << "Move Face did not adopt";
    }
    {   // Rejects when the body changed under it (fresh Document, so this op
        // has no minted face name yet - matches TaperOp's equivalent test).
        Document doc2; const int id2 = doc2.addBody(base, "plate");
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(id2);
        op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Translate);
        op->setMoveVector(gp_Vec(2.0, 0.0, 0.0));
        op->setPrecomputedResult(BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape(),
                                 marker(), op->previewKey(base));
        EXPECT_TRUE(op->execute(doc2)) << "Move Face's fallback computation failed";
        EXPECT_FALSE(isMarker(doc2.getBody(id2))) << "Move Face adopted a stale result";
    }
}

TEST(PreviewAdoption, MoveFaceOneShotConsumptionAndUndo) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto op = std::make_unique<MoveFaceOp>();
    op->setBody(id);
    op->setFace(f);
    op->setKind(MoveFaceOp::Kind::Translate);
    op->setMoveVector(gp_Vec(2.0, 0.0, 0.0));
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));

    ASSERT_TRUE(op->execute(doc));
    ASSERT_TRUE(isMarker(doc.getBody(id))) << "did not adopt on first execute";

    ASSERT_TRUE(op->undo(doc));
    EXPECT_TRUE(doc.getBody(id).IsEqual(base)) << "undo did not restore the previous shape";
}

TEST(PreviewAdoption, MoveFaceDifferentKindsNeverCrossAdopt) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Face f = topFace(base);

    auto translate = std::make_unique<MoveFaceOp>();
    translate->setBody(0);
    translate->setFace(f);
    translate->setKind(MoveFaceOp::Kind::Translate);
    translate->setMoveVector(gp_Vec(2.0, 0.0, 0.0));

    auto rotate = std::make_unique<MoveFaceOp>();
    rotate->setBody(0);
    rotate->setFace(f);
    rotate->setKind(MoveFaceOp::Kind::Rotate);
    rotate->setRotation(gp_Dir(1.0, 0.0, 0.0), 2.0); // numerically unrelated to
                                                      // Translate's key on purpose -
                                                      // the kind tag alone must
                                                      // already make these differ
    const std::string keyT = translate->previewKey(base);
    const std::string keyR = rotate->previewKey(base);
    ASSERT_FALSE(keyT.empty());
    ASSERT_FALSE(keyR.empty());
    EXPECT_NE(keyT, keyR) << "Translate and Rotate must never produce the same key";
}

// Table-driven: every field previewKey() reads must actually change the key
// when it changes, for every Kind branch - an omitted field would let a
// changed-but-unkeyed parameter silently adopt a stale shape.
TEST(PreviewAdoption, MoveFacePreviewKeyIsSensitiveToEveryField) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Face f = topFace(base);
    // A second, distinct face on the same base, for the face-selection case.
    TopoDS_Face f2;
    { double bestZ = -1e300;
      for (TopExp_Explorer e(base, TopAbs_FACE); e.More(); e.Next()) {
          if (TopoDS::Face(e.Current()).IsSame(f)) continue;
          GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
          if (g.Mass() > bestZ) { bestZ = g.Mass(); f2 = TopoDS::Face(e.Current()); }
      }
      ASSERT_FALSE(f2.IsNull());
    }
    std::vector<bool> hs9(9, false), hv9(9, false);

    auto baseline = [&] {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0);
        op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Translate);
        op->setMoveVector(gp_Vec(1.0, 0.0, 0.0));
        op->setLoopMotion(true, hs9, hv9);
        return op;
    };
    const std::string baseKey = baseline()->previewKey(base);
    ASSERT_FALSE(baseKey.empty());

    struct Case { const char* name; std::function<void(MoveFaceOp&)> mutate; };
    std::vector<Case> cases = {
        {"move vector", [](MoveFaceOp& op) { op.setMoveVector(gp_Vec(1.0, 0.0, 1e-6)); }},
        {"moveOuter", [&](MoveFaceOp& op) { op.setLoopMotion(false, hs9, hv9); }},
        {"holeSlant", [&](MoveFaceOp& op) {
            std::vector<bool> hs = hs9; hs[0] = true;
            op.setLoopMotion(true, hs, hv9);
        }},
        {"holeVertical", [&](MoveFaceOp& op) {
            std::vector<bool> hv = hv9; hv[0] = true;
            op.setLoopMotion(true, hs9, hv);
        }},
        {"face selection", [&](MoveFaceOp& op) { op.setFace(f2); }},
    };
    for (const auto& c : cases) {
        auto op = baseline();
        c.mutate(*op);
        const std::string k = op->previewKey(base);
        EXPECT_NE(k, baseKey) << "field not reflected in key: " << c.name;
    }

    // Rotate: implicit axis, implicit angle, explicit transform, and the
    // switch between implicit/explicit must all key differently.
    {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0); op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Rotate);
        op->setRotation(gp_Dir(1, 0, 0), 0.1);
        const std::string k1 = op->previewKey(base);
        op->setRotation(gp_Dir(0, 1, 0), 0.1); // axis changed, angle unchanged
        const std::string k2 = op->previewKey(base);
        EXPECT_NE(k1, k2) << "Rotate axis not reflected in key";
        op->setRotation(gp_Dir(0, 1, 0), 0.2); // angle changed, axis unchanged
        const std::string k3 = op->previewKey(base);
        EXPECT_NE(k2, k3) << "Rotate angle not reflected in key";
        op->setRotationExplicit(gp_Trsf());
        const std::string k4 = op->previewKey(base);
        EXPECT_NE(k3, k4) << "switching implicit/explicit Rotate not reflected in key";
        gp_Trsf t2; t2.SetTranslation(gp_Vec(0, 0, 1));
        op->setRotationExplicit(t2);
        const std::string k5 = op->previewKey(base);
        EXPECT_NE(k4, k5) << "explicit rotation transform not reflected in key";
    }
    // Twist.
    {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0); op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Twist);
        op->setTwist(0.1);
        const std::string k1 = op->previewKey(base);
        op->setTwist(0.2);
        EXPECT_NE(k1, op->previewKey(base)) << "Twist angle not reflected in key";
    }
    // Scale: uniform factor, the switch to non-uniform, and BOTH non-uniform
    // axes and BOTH non-uniform factors independently.
    {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0); op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Scale);
        op->setScaleFactor(1.1);
        const std::string k1 = op->previewKey(base);
        op->setScaleFactor(1.2);
        const std::string k2 = op->previewKey(base);
        EXPECT_NE(k1, k2) << "uniform scale factor not reflected in key";

        op->setScaleNonUniform(gp_Dir(1, 0, 0), gp_Dir(0, 1, 0), 1.1, 1.3);
        const std::string k3 = op->previewKey(base);
        EXPECT_NE(k2, k3) << "switching uniform/non-uniform scale not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(0, 1, 0), 1.1, 1.3); // axis A changed
        const std::string k4 = op->previewKey(base);
        EXPECT_NE(k3, k4) << "non-uniform scale axis A not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(1, 0, 0), 1.1, 1.3); // axis B changed
        const std::string k5 = op->previewKey(base);
        EXPECT_NE(k4, k5) << "non-uniform scale axis B not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(1, 0, 0), 1.2, 1.3); // factor A changed
        const std::string k6 = op->previewKey(base);
        EXPECT_NE(k5, k6) << "non-uniform scale factor A not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(1, 0, 0), 1.2, 1.4); // factor B changed
        EXPECT_NE(k6, op->previewKey(base)) << "non-uniform scale factor B not reflected in key";
    }
}
```

Add `#include <functional>` to the top of `tests/test_preview_adoption.cpp` if not already present (needed for `std::function` in the `Case` struct above).

### Step 3: Run the tests to verify they fail

```bash
cmake --build build --target test_preview_adoption -j8
./build/tests/test_preview_adoption --gtest_filter='PreviewAdoption.MoveFace*'
```

Expected: compile failure (`previewKey` not declared as a member `MoveFaceOp` overrides - the base class's default returns `{}`, so `MoveFaceAdoptsAndRejectsOnTheSameTerms` will actually compile and RUN, but fail at `ASSERT_TRUE(op->execute(doc))`/`isMarker` since `canAdopt` never fires when the key is always empty). Confirm the actual failure mode you see and note it - either a build failure (if you haven't added the header declaration yet) or a runtime `EXPECT_TRUE(isMarker(...))` failure (once the header declaration exists but `execute()` doesn't check it yet) is an acceptable "fails for the right reason."

### Step 4: Implement `previewKey()`

Add this to `src/modeling/MoveFaceOp.cpp`, anywhere after `execute()`/`undo()` (e.g. right before `description()`'s implementation - place it near the other small non-`execute` methods):

```cpp
std::string MoveFaceOp::previewKey(const TopoDS_Shape& base) const {
    // %a for every double: a decimal-rounded key (%f/%g) would let a
    // 1e-15-different rotation falsely adopt a stale shape. Built as a
    // std::string, not a fixed buffer - Rotate's explicit-transform case
    // alone is 12 doubles, well past what a small snprintf buffer holds
    // without silent truncation (see TaperOp::previewKey's identical note).
    auto hex = [](double v) {
        char b[32];
        std::snprintf(b, sizeof(b), "%a", v);
        return std::string(b);
    };
    // The kind tag goes first and is never omitted: two different Kinds must
    // never produce the same key even if their numeric fields coincide.
    std::string head = "moveface;k=" + std::to_string(static_cast<int>(m_kind));
    switch (m_kind) {
        case Kind::Translate:
            head += ";mv=" + hex(m_move.X()) + "," + hex(m_move.Y()) + "," + hex(m_move.Z());
            break;
        case Kind::Rotate:
            head += ";re=" + std::string(m_rotUseExplicit ? "1" : "0");
            if (m_rotUseExplicit) {
                gp_Trsf t = m_rotExplicit;
                for (int r = 1; r <= 3; ++r)
                    for (int c = 1; c <= 4; ++c)
                        head += "," + hex(t.Value(r, c));
            } else {
                head += ";ax=" + hex(m_rotAxis.X()) + "," + hex(m_rotAxis.Y()) + "," +
                        hex(m_rotAxis.Z()) + ";an=" + hex(m_rotAngle);
            }
            break;
        case Kind::Twist:
            head += ";tw=" + hex(m_twistAngle);
            break;
        case Kind::Scale:
            head += ";sn=" + std::string(m_scaleNonUniform ? "1" : "0");
            if (m_scaleNonUniform) {
                head += ";aA=" + hex(m_scaleAxisA.X()) + "," + hex(m_scaleAxisA.Y()) + "," +
                        hex(m_scaleAxisA.Z()) +
                        ";aB=" + hex(m_scaleAxisB.X()) + "," + hex(m_scaleAxisB.Y()) + "," +
                        hex(m_scaleAxisB.Z()) +
                        ";sA=" + hex(m_scaleA) + ";sB=" + hex(m_scaleB);
            } else {
                head += ";sf=" + hex(m_scaleFactor);
            }
            break;
    }
    head += ";mo=" + std::string(m_moveOuter ? "1" : "0") + ";hs=";
    for (bool b : m_holeSlant) head += (b ? '1' : '0');
    head += ";hv=";
    for (bool b : m_holeVertical) head += (b ? '1' : '0');
    head += ";face=";

    std::vector<TopoDS_Shape> faces{m_face};
    std::string sel;
    if (!SubShapeIndex::orientedKey(base, faces, TopAbs_FACE, sel)) return {};
    return head + sel;
}
```

### Step 5: Extract the shared tail into `applyResult()`

In `src/modeling/MoveFaceOp.cpp`, find the block at the end of `execute()` (currently, before this task, lines 520-544 - re-locate by searching for `m_resultShape = result;`, since line numbers will have shifted after Step 4's insertion). Replace:

```cpp
        m_resultShape = result;
        doc.updateBody(m_bodyId, result);

        // Move on-face sketches by the SAME transform (slide / tilt / scale), so
        // they stay glued to the face - but only when the face OUTLINE moves
        // (sketches ride the face, not a hole). Stored for undo.
        m_appliedXform = m_moveOuter ? topT : gp_Trsf();
        if (m_moveOuter)
        for (int sid : m_sketchIds) {
            if (auto sk = doc.getSketch(sid)) {
                gp_Pln pln = sk->getPlane();
                pln.Transform(m_appliedXform);
                sk->setPlane(pln);
                TopoDS_Face sf = sk->getSourceFace();
                if (!sf.IsNull()) {
                    TopoDS_Shape mv = BRepBuilderAPI_Transform(sf, m_appliedXform, Standard_True).Shape();
                    if (!mv.IsNull() && mv.ShapeType() == TopAbs_FACE)
                        sk->setSourceFace(TopoDS::Face(mv));
                }
            }
        }
        return true;
```

with:

```cpp
        applyResult(doc, result, topT);
        return true;
```

Then add the new method, anywhere after `execute()`'s closing brace:

```cpp
void MoveFaceOp::applyResult(Document& doc, const TopoDS_Shape& result,
                             const gp_Trsf& topT) {
    m_resultShape = result;
    doc.updateBody(m_bodyId, result);

    // Move on-face sketches by the SAME transform (slide / tilt / scale), so
    // they stay glued to the face - but only when the face OUTLINE moves
    // (sketches ride the face, not a hole). Stored for undo.
    m_appliedXform = m_moveOuter ? topT : gp_Trsf();
    if (m_moveOuter)
    for (int sid : m_sketchIds) {
        if (auto sk = doc.getSketch(sid)) {
            gp_Pln pln = sk->getPlane();
            pln.Transform(m_appliedXform);
            sk->setPlane(pln);
            TopoDS_Face sf = sk->getSourceFace();
            if (!sf.IsNull()) {
                TopoDS_Shape mv = BRepBuilderAPI_Transform(sf, m_appliedXform, Standard_True).Shape();
                if (!mv.IsNull() && mv.ShapeType() == TopAbs_FACE)
                    sk->setSourceFace(TopoDS::Face(mv));
            }
        }
    }
}
```

### Step 5b: Add the static counters

`MoveFacePreviewJob::run()` executes `MoveFaceOp::execute()` on a background `std::thread` (that is the entire point of the async preview), while `commitMoveFace()`'s `pushOperation` runs another `execute()` on the main thread, and a test reads the counters from the main thread too - plain `int`s here would be a real data race (undefined behavior), not just a style nit. Use `std::atomic<int>`.

In `src/modeling/MoveFaceOp.cpp`, add near the top (after the includes, before the first function):

```cpp
namespace {
std::atomic<int> g_moveFaceAdoptedCount{0};
std::atomic<int> g_moveFaceRecomputedCount{0};
} // namespace

int MoveFaceOp::adoptedCount() { return g_moveFaceAdoptedCount.load(std::memory_order_relaxed); }
int MoveFaceOp::recomputedCount() { return g_moveFaceRecomputedCount.load(std::memory_order_relaxed); }
```

Add `#include <atomic>` and `#include "../core/Verbose.h"` to `MoveFaceOp.cpp`'s include block (check both first - neither is currently included; match this codebase's established relative-include style for `src/modeling/*.cpp`, e.g. `MoveHoleOp.cpp`/`FilletOp.cpp` both use `"../core/Verbose.h"`, not `"core/Verbose.h"`).

### Step 6: Add the `takePrecomputed()` call and the adopt branch

At the very top of `execute()`, before the early-return guards (currently the first line inside the function body, `if (m_bodyId < 0 || m_face.IsNull()) return false;`), add:

```cpp
    auto adopted = takePrecomputed();
```

So the guards read:

```cpp
bool MoveFaceOp::execute(Document& doc) {
    auto adopted = takePrecomputed();
    if (m_bodyId < 0 || m_face.IsNull()) return false;
    ...
```

This must be consumed even on an early return: a one-shot candidate that survives a refused `execute()` (invalid params) would otherwise leak into a LATER `execute()` at different params and adopt a result that doesn't belong to it - the exact bug `ShellOp` already guards against with the same ordering.

Then, immediately before the line that currently reads (after Step 4/5's edits, still the same statement):

```cpp
        TopoDS_Shape newFeature = isTwist ? buildTwistFeature(true) : buildFeature(true);
```

insert the adopt check:

```cpp
        // The preview worker already loft-and-cut this exact face with these
        // exact parameters: adopt its result instead of repeating the
        // per-hole loft/cut work below, which is what makes Move Face slow
        // on a many-hole face. Checked here (after the face re-bind above,
        // after topT/twAxis are fully resolved for every Kind including
        // Twist at line ~429) and NOT any earlier, because applyResult()
        // needs a fully-resolved topT for the sketch-slide, and the adopt
        // key needs the RE-BOUND m_face, not the pre-rebind handle.
        if (adopted && canAdopt(*adopted, m_previousShape, previewKey(m_previousShape))) {
            g_moveFaceAdoptedCount.fetch_add(1, std::memory_order_relaxed);
            if (materializr::isVerbose())
                std::fprintf(stderr, "[MoveFace] adopted precomputed result, skipped recompute\n");
            applyResult(doc, adopted->result, topT);
            return true;
        }
        g_moveFaceRecomputedCount.fetch_add(1, std::memory_order_relaxed);
        if (materializr::isVerbose()) {
            if (adopted)
                std::fprintf(stderr, "[MoveFace] a precomputed candidate was offered but its "
                                      "key/base no longer matched - recomputing\n");
            else
                std::fprintf(stderr, "[MoveFace] no precomputed candidate - recomputing\n");
        }

        TopoDS_Shape newFeature = isTwist ? buildTwistFeature(true) : buildFeature(true);
```

### Step 7: Run the tests to verify they pass

```bash
cmake --build build --target test_preview_adoption -j8
./build/tests/test_preview_adoption --gtest_filter='PreviewAdoption.MoveFace*'
```

Expected: `PASSED` for all three new tests (`MoveFaceAdoptsAndRejectsOnTheSameTerms`, `MoveFaceOneShotConsumptionAndUndo`, `MoveFaceDifferentKindsNeverCrossAdopt`).

### Step 8: Run the full `test_preview_adoption` suite and `test_move_face_preview`/`test_move_face_async` to confirm no regression

```bash
cmake --build build --target test_preview_adoption test_move_face_preview test_move_face_async -j8
./build/tests/test_preview_adoption
./build/tests/test_move_face_preview
./build/tests/test_move_face_async
```

Expected: every existing test in all three binaries still passes (this task changed `execute()`'s control flow but not its recompute path's output for any call that doesn't set a `Precomputed` candidate - `test_move_face_preview`/`test_move_face_async` never call `setPrecomputedResult`, so they must be byte-for-byte unaffected).

### Step 9: Commit

```bash
git add src/modeling/MoveFaceOp.h src/modeling/MoveFaceOp.cpp tests/test_preview_adoption.cpp
git commit -m "MoveFaceOp: add Precomputed/canAdopt support (op-level only, not wired to commit yet)"
```

---

## Task 2: `MoveFacePreviewJob` computes and returns the preview's key

**Files:**
- Modify: `src/app/MoveFacePreview.h`
- Modify: `src/app/MoveFacePreview.cpp`
- Test: `tests/test_move_face_preview.cpp` (extend)

**Interfaces:**
- Consumes: `MoveFaceOp::previewKey(const TopoDS_Shape&) const` from Task 1.
- Produces: `MoveFacePreviewResult::key` (a `std::string`, empty when the run failed or the op declined to key itself) - Task 3's `MoveFaceController::pollPreview` reads this.

### Step 1: Write the failing test

`tests/test_move_face_preview.cpp` already has a `makeHolePlate(int n, ...)` + `topFace(const TopoDS_Shape&)` helper pair in its anonymous namespace (used by e.g. `TranslateMatchesADirectExecuteAndLeavesTheLiveDocumentAlone`). Reuse them exactly. Add this test after the existing ones:

```cpp
TEST(MoveFacePreview, ResultCarriesAKeyMatchingTheOpsOwnPreviewKey) {
    TopoDS_Shape body = makeHolePlate(3);
    TopoDS_Face face = topFace(body);
    ASSERT_FALSE(face.IsNull());

    auto configure = [](MoveFaceOp& op) {
        op.setKind(MoveFaceOp::Kind::Translate);
        op.setMoveVector(gp_Vec(2.0, 0.0, 0.0));
        op.setLoopMotion(true, std::vector<bool>(9, false), std::vector<bool>(9, false));
    };

    std::unique_ptr<MoveFacePreviewJob> job = MoveFacePreviewJob::prepare(body, face, configure);
    ASSERT_TRUE(job);
    MoveFacePreviewResult result = job->run();
    ASSERT_TRUE(result.ok);

    // Not just non-empty: must equal what previewKey() computes independently
    // for an equivalently-configured op on the same base - a non-empty but
    // WRONG key (e.g. one that omitted a field) would still pass a
    // non-empty check and silently defeat adoption's safety argument.
    MoveFaceOp equivalent;
    equivalent.setFace(face);
    equivalent.setKind(MoveFaceOp::Kind::Translate);
    equivalent.setMoveVector(gp_Vec(2.0, 0.0, 0.0));
    equivalent.setLoopMotion(true, std::vector<bool>(9, false), std::vector<bool>(9, false));
    EXPECT_EQ(result.key, equivalent.previewKey(body));
}
```

### Step 2: Run to verify it fails

```bash
cmake --build build --target test_move_face_preview -j8
./build/tests/test_move_face_preview --gtest_filter='MoveFacePreview.ResultCarriesAKeyMatchingTheOpsOwnPreviewKey'
```

Expected: compile failure - `MoveFacePreviewResult` has no `key` member yet.

### Step 3: Add the `key` field and a stored pre-execute base shape

In `src/app/MoveFacePreview.h`, add to `MoveFacePreviewResult` (after the existing `millis` field):

```cpp
struct MoveFacePreviewResult {
    bool ok = false;
    TopoDS_Shape shape;
    double millis = 0.0; // execute() wall time on the worker, for diagnostics
    // The op's own previewKey(), evaluated against the worker's pre-execute
    // scratch base AFTER execute() succeeds - so it reflects the op's own
    // face re-bind, exactly like SnapshotPreviewJob::run() does for
    // Shell/Taper/ScaleFace (see src/app/SnapshotPreview.cpp). Empty when
    // the run failed or the op's previewKey() couldn't resolve a selection.
    std::string key;
};
```

Add `#include <string>` to the top of `MoveFacePreview.h` if not already present via a transitive include (check first; `<functional>`/`<memory>` are already there but neither guarantees `<string>`).

Add a new private member to `MoveFacePreviewJob` (in the `private:` section, alongside `m_scratch`/`m_scratchBodyId`/`m_op`):

```cpp
    TopoDS_Shape m_scratchBase; // the scratch body's shape BEFORE execute() ran,
                               // for previewKey() to key against post-execute
```

### Step 4: Populate it in `prepare()` and `run()`

In `src/app/MoveFacePreview.cpp`'s `prepare()`, right after `job->m_scratchBodyId = job->m_scratch->addBody(copier.Shape(), "preview");`, add:

```cpp
        job->m_scratchBase = copier.Shape();
```

In `run()`, right after `r.ok = m_op->execute(*m_scratch);` succeeds (i.e., inside the existing `if (!r.ok) return r;` guard's else-continuation - the code already falls through to fetch `r.shape` only when `r.ok` is true), add the key computation right after `r.shape = m_scratch->getBody(m_scratchBodyId);` succeeds:

```cpp
    try {
        r.shape = m_scratch->getBody(m_scratchBodyId);
    } catch (...) {
        r.ok = false;
        return r;
    }
    r.key = m_op->previewKey(m_scratchBase);
    return r;
```

(This replaces the existing `catch (...) { r.ok = false; } return r;` tail - note the added early `return r;` inside the catch, so `r.key` is never computed against a shape that failed to fetch.)

### Step 5: Run to verify it passes

```bash
cmake --build build --target test_move_face_preview -j8
./build/tests/test_move_face_preview
```

Expected: the new test passes, and every pre-existing test in this file still passes (none of them read `.key`, so adding it is additive).

### Step 6: Commit

```bash
git add src/app/MoveFacePreview.h src/app/MoveFacePreview.cpp tests/test_move_face_preview.cpp
git commit -m "MoveFacePreviewJob: compute and return the op's previewKey"
```

---

## Task 3: `MoveFaceController` caches the landed preview and offers it at commit

**Files:**
- Modify: `src/app/FaceOpControllers.h`
- Modify: `src/app/FaceOpControllers.cpp`
- Test: `tests/test_move_face_async.cpp` (extend, if it can reach `commitMoveFace()` directly - see Step 5; otherwise this task's safety net is Task 1's op-level tests plus the manual smoke test in Task 4)

**Interfaces:**
- Consumes: `MoveFacePreviewResult::key`/`::shape` (Task 2), `Operation::setPrecomputedResult` (already exists, used directly).
- Produces: nothing new for later tasks - this is the last wiring step.

### Step 1: Add the three new members to `MoveFaceController`

In `src/app/FaceOpControllers.h`, in the `private:` section, right after the existing (currently lines 300-303):

```cpp
    PreviewDispatch<MoveFaceKey> m_mfDispatch;
    AsyncJob<MoveFacePreviewResult> m_mfJob;
    std::unique_ptr<MoveFacePreviewJob> m_mfPendingJob;
    MoveFaceKey m_mfPendingKey;
```

add:

```cpp
    // The last off-thread preview that landed and was applied to the live
    // document, plus what it was computed from - offered to the commit op in
    // commitMoveFace(), which re-checks both itself before adopting (see
    // Operation::canAdopt). Cleared in beginMoveFace() and in
    // commitMoveFace() so a landed shape from one gesture can never leak
    // into the next one.
    TopoDS_Shape m_landedShape;
    TopoDS_Shape m_landedBase;
    std::string m_landedPreviewKey;
```

### Step 2: Populate them in `pollPreview()`

In `src/app/FaceOpControllers.cpp`, find `MoveFaceController::pollPreview` (currently lines 1634-1657):

```cpp
void MoveFaceController::pollPreview(const IopContext& ctx) {
    m_mfJob.reap();
    std::optional<MoveFacePreviewResult> result = m_mfJob.take();
    if (result) {
        const bool wanted = m_st.moveFaceActive && !m_st.moveHoleMode &&
                            !localTweakApplies() && faceXformNontrivial();
        if (wanted) {
            const MoveFaceKey now = currentMoveFaceKey();
            if (m_mfDispatch.finished(now) && result->ok && !result->shape.IsNull()) {
                ctx.doc.updateBody(m_st.moveFaceBodyId, result->shape);
                ctx.markMeshesDirty();
            }
        }
    }
    ...
```

Change the body of the `if (m_mfDispatch.finished(now) && result->ok && !result->shape.IsNull())` block to also cache the landed state:

```cpp
            if (m_mfDispatch.finished(now) && result->ok && !result->shape.IsNull()) {
                ctx.doc.updateBody(m_st.moveFaceBodyId, result->shape);
                ctx.markMeshesDirty();
                // The base this result was computed from is the gesture's own
                // snapshot - it does not change mid-gesture (a new gesture
                // resets it in beginMoveFace), so it is safe to read here.
                m_landedShape = result->shape;
                m_landedBase = m_st.moveFacePreviousShape;
                m_landedPreviewKey = result->key;
            }
```

### Step 3: Clear them in `beginMoveFace()` AND `cancelMoveFace()`

Two gesture-ending paths reset `m_mfJob`/`m_mfDispatch`/`m_mfPendingJob` today and must clear the landed cache the same way, or a large OCCT shape graph lingers in memory until the NEXT gesture happens to start (or indefinitely, if the user never starts another Move Face gesture this session).

In `src/app/FaceOpControllers.cpp`, find `MoveFaceController::beginMoveFace` (currently line 1172), and its reset block (currently lines 1193-1195):

```cpp
    m_mfJob.abandon(); // a job from the PREVIOUS gesture must never land into this one
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
```

Add right after it:

```cpp
    m_landedShape.Nullify();
    m_landedBase.Nullify();
    m_landedPreviewKey.clear();
```

Find `MoveFaceController::cancelMoveFace` (currently line 1887) and its identical reset block (currently lines 1888-1890):

```cpp
    m_mfJob.abandon();
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
```

Add the same three clearing lines right after it.

### Step 4: Offer the landed state in `commitMoveFace()`

In `src/app/FaceOpControllers.cpp`, find `MoveFaceController::commitMoveFace` (currently lines 1774-1885). Its reset block at the top (currently lines 1775-1778):

```cpp
    m_mfJob.abandon();
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
    if (!m_st.moveFaceActive) { return; }
```

`commitMoveFace()` has multiple exit paths after this point (the `moveHoleMode` branch returns early around line 1798; the `localTweakApplies()` branch builds a `FaceTweakOp`, not a `MoveFaceOp`; the general branch is the one that can adopt; and a fourth path exists where `faceXformNontrivial()` is false and NEITHER branch's `if` fires, so `committed` stays `false` and control falls through to the cleanup at the bottom). The landed cache must not survive ANY of these - capture it into locals and clear the members UNCONDITIONALLY, right after the existing reset block, before any of the branches:

```cpp
    m_mfJob.abandon();
    m_mfDispatch.reset();
    m_mfPendingJob.reset();
    // Captured once, up front, so every one of this function's several exit
    // paths below (hole-move return, local-tweak branch, general branch, or
    // a no-op fall-through when faceXformNontrivial() is false) consumes the
    // cache exactly once. A stale landed shape must not survive into the
    // NEXT gesture (cleared here) or be silently re-offered to an unrelated
    // later op (cleared here too, not just in beginMoveFace()/cancelMoveFace()).
    const TopoDS_Shape landedShapeForCommit = m_landedShape;
    const TopoDS_Shape landedBaseForCommit = m_landedBase;
    const std::string landedKeyForCommit = m_landedPreviewKey;
    m_landedShape.Nullify();
    m_landedBase.Nullify();
    m_landedPreviewKey.clear();
    if (!m_st.moveFaceActive) { return; }
```

Find the `MoveFaceOp` construction block (currently lines 1823-1829):

```cpp
    } else if (faceXformNontrivial() && m_st.moveFaceBodyId >= 0 && !m_st.moveFaceFace.IsNull()) {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(m_st.moveFaceBodyId);
        op->setFace(m_st.moveFaceFace);
        configureFaceOp(*op);
        op->setSketchIds(m_st.moveFaceSketchIds); // on-face sketches ride along
        committed = ctx.history.pushOperation(std::move(op), ctx.doc);
```

Insert the offer between `configureFaceOp(*op);` and `op->setSketchIds(...)`, using the locals captured above:

```cpp
    } else if (faceXformNontrivial() && m_st.moveFaceBodyId >= 0 && !m_st.moveFaceFace.IsNull()) {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(m_st.moveFaceBodyId);
        op->setFace(m_st.moveFaceFace);
        configureFaceOp(*op);
        // The worker may already have computed exactly this. Offer it; the op
        // adopts only if the live body is still the one the worker started
        // from AND its own post-rebind selection matches - see
        // Operation::canAdopt. m_mfJob.abandon() above parks (does not land)
        // any job still in flight, so a Confirm click that races a fresher
        // preview correctly falls back to whatever was last LANDED, and its
        // own key check decides whether that's still valid - no wait, no race.
        // This is a pre-existing property of this controller (today's
        // synchronous commit already runs concurrently with an abandoned
        // in-flight worker in the same case) - adoption makes it fire LESS
        // often, never more, by skipping the fallback whenever the landed
        // result still matches. Whether it actually adopts or falls back to
        // a full recompute is logged from inside MoveFaceOp::execute() itself
        // (see Step 6), not here - only execute() knows the real decision.
        if (!landedShapeForCommit.IsNull() && !landedBaseForCommit.IsNull() &&
            !landedKeyForCommit.empty())
            op->setPrecomputedResult(landedBaseForCommit, landedShapeForCommit, landedKeyForCommit);
        op->setSketchIds(m_st.moveFaceSketchIds); // on-face sketches ride along
        committed = ctx.history.pushOperation(std::move(op), ctx.doc);
```

### Step 5: Two required controller-level tests

**Verified reachable** - `tests/test_move_face_async.cpp` already constructs a real `MoveFaceController mfc;` + `IopContext ctx = h.ctx();` via its `Harness` struct, sets gesture state directly through `mfc.st()`, and an existing test (`CommitAbandonsAnInFlightJobInsteadOfLeavingItAppliedLater`, line 266) already calls `mfc.commitMoveFace(ctx)` directly. Both tests below are REQUIRED, not optional - timing cannot prove adoption happened (scheduler load, OCCT internal caching, and fixture variance all move independently of whether the adopt branch ran), so both use `TopoDS_Shape::IsEqual`/geometric-recompute comparisons instead, matching this file's own existing idiom (see `afterCommit.IsEqual(...)` at line 313 and the volume/centroid comparisons in `ReleaseAfterADragReturnsWellUnderTheOpsRealCost`).

Add both after the existing tests in `tests/test_move_face_async.cpp`:

```cpp
TEST(MoveFaceAsync, CommitAdoptsTheLandedPreviewWithoutRecomputing) {
    Harness h;
    TopoDS_Shape body = makeHolePlate(5); // 25 holes
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

    mfc.updateMoveFace(ctx);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_FALSE(mfc.previewPending()) << "worker never landed within 5s";

    // The exact TShape the worker landed - a fresh recompute would produce a
    // DIFFERENT TShape object even if geometrically identical, so IsEqual
    // below is direct, non-flaky proof that adoption (not recomputation)
    // happened. Corroborated by MoveFaceOp's own counters (round 2 Codex
    // finding 6) so the proof does not rest on IsEqual/TShape-identity
    // reasoning alone.
    const TopoDS_Shape landed = h.doc.getBody(bodyId);
    const int adoptedBefore = MoveFaceOp::adoptedCount();
    const int recomputedBefore = MoveFaceOp::recomputedCount();

    mfc.commitMoveFace(ctx);
    // commitMoveFace restores m_st.moveFacePreviousShape before the real op
    // runs, then pushOperation re-executes it - the committed shape below is
    // whatever that execute() actually produced.
    const TopoDS_Shape committed = h.doc.getBody(bodyId);
    EXPECT_TRUE(committed.IsEqual(landed))
        << "commit did not adopt the landed preview - it recomputed instead";
    EXPECT_EQ(MoveFaceOp::adoptedCount(), adoptedBefore + 1);
    EXPECT_EQ(MoveFaceOp::recomputedCount(), recomputedBefore)
        << "the expensive recompute path ran even though adoption should have skipped it";
}

TEST(MoveFaceAsync, CommitFallsBackToRecomputeWhenParamsChangedAfterLanding) {
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
    mfc.st().moveFaceVec = glm::vec3(1.0f, 0.0f, 0.0f);
    mfc.st().moveFaceMoveOuter = true;
    mfc.st().moveFaceHoleSlant.assign(25, false);
    mfc.st().moveFaceHoleVertical.assign(25, false);

    mfc.updateMoveFace(ctx);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mfc.previewPending() && std::chrono::steady_clock::now() < deadline) {
        mfc.pollPreview(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ASSERT_FALSE(mfc.previewPending()) << "worker never landed within 5s";
    const TopoDS_Shape landedAtV1 = h.doc.getBody(bodyId);
    const int adoptedBefore = MoveFaceOp::adoptedCount();
    const int recomputedBefore = MoveFaceOp::recomputedCount();
    const int stepBefore = ctx.history.currentStep();

    // Simulate "Confirm clicked before the newer preview for a changed
    // parameter landed": change the vector WITHOUT launching/landing a new
    // preview, so the cached m_landedPreviewKey still describes V1.
    mfc.st().moveFaceVec = glm::vec3(2.0f, 0.0f, 0.0f);
    mfc.commitMoveFace(ctx);
    const TopoDS_Shape committed = h.doc.getBody(bodyId);

    // The commit actually pushed a step (not a silently-refused no-op) and
    // took the fallback path, not adoption.
    EXPECT_EQ(ctx.history.currentStep(), stepBefore + 1)
        << "commit did not push a step";
    EXPECT_EQ(MoveFaceOp::adoptedCount(), adoptedBefore)
        << "commit adopted a stale candidate instead of falling back";
    EXPECT_EQ(MoveFaceOp::recomputedCount(), recomputedBefore + 1);
    EXPECT_FALSE(committed.IsEqual(landedAtV1))
        << "commit adopted a shape computed for the WRONG parameters";

    // Must be geometrically IDENTICAL (not just approximately so - volume
    // and centroid alone are weak for perforated geometry, per round 2
    // Codex finding 7) to an independent direct execute() at V2: the
    // symmetric difference of the two shapes must have zero volume.
    Document ref;
    int refId = ref.addBody(body, "plate");
    MoveFaceOp direct;
    direct.setBody(refId);
    direct.setFace(topFace(ref.getBody(refId)));
    direct.setKind(MoveFaceOp::Kind::Translate);
    direct.setMoveVector(gp_Vec(2.0, 0.0, 0.0));
    direct.setLoopMotion(true, std::vector<bool>(25, false), std::vector<bool>(25, false));
    ASSERT_TRUE(direct.execute(ref));
    const TopoDS_Shape refShape = ref.getBody(refId);
    BRepAlgoAPI_Cut cutA(committed, refShape);
    ASSERT_TRUE(cutA.IsDone());
    ASSERT_FALSE(cutA.Shape().IsNull());
    BRepAlgoAPI_Cut cutB(refShape, committed);
    ASSERT_TRUE(cutB.IsDone());
    ASSERT_FALSE(cutB.Shape().IsNull());
    EXPECT_NEAR(volume(cutA.Shape()), 0.0, 1e-6);
    EXPECT_NEAR(volume(cutB.Shape()), 0.0, 1e-6);
}
```

### Step 6: Build and run every Move-Face-related test

```bash
cmake --build build --target test_preview_adoption test_move_face_preview test_move_face_async -j8
./build/tests/test_preview_adoption
./build/tests/test_move_face_preview
./build/tests/test_move_face_async
```

Expected: all pass, including anything added in Step 5.

### Step 7: Full project build and full test suite

```bash
cmake --build build -j8
cd build && ctest --output-on-failure -j8
```

Expected: full app build succeeds, 100% of the ctest binaries pass (this repo's baseline is 113/113 as of the last full run this session - confirm the count matches or explain any change).

### Step 8: Run this project's style gates

```bash
python3 tools/no_em_dashes.py
python3 tools/no_double_build.py
python3 tools/units_audit.py
```

Expected: all three clean (no new mm-literal or numeric-control introduced by this change; no double-built booleans - this plan introduces none).

### Step 9: Commit

```bash
git add src/app/FaceOpControllers.h src/app/FaceOpControllers.cpp tests/test_move_face_async.cpp
git commit -m "MoveFaceController: adopt the landed preview at commit instead of recomputing"
```

---

## Task 4: Benchmark and close out

**Files:**
- Create (throwaway, NOT committed): a bench harness mirroring `movefaceop-freeze.md`'s described `tests/bench_move_face.cpp` pattern.
- Modify (memory, not code): `~/.claude/projects/-Users-laptop-Documents-Coding-projects-Materialzr/memory/movefaceop-freeze.md`

**Interfaces:**
- Consumes: everything from Tasks 1-3.
- Produces: the closing measurement for this plan's own report; nothing downstream depends on this task's artifacts.

This task's job is narrower than Task 3's new tests: Task 3 already proves adoption is WIRED correctly end-to-end (`CommitAdoptsTheLandedPreviewWithoutRecomputing`) and that the fallback is CORRECT (`CommitFallsBackToRecomputeWhenParamsChangedAfterLanding`). This task exists only to produce the ACTUAL before/after millisecond numbers for the historical 14967.6 ms/400-hole figure in `movefaceop-freeze.md`, measured the same way that figure was measured (direct `MoveFaceOp::execute()` timing, not full-controller overhead) so the two numbers are comparable.

### Step 0: Confirm the target file is safe to revert

```bash
git status --porcelain tests/CMakeLists.txt
git diff tests/CMakeLists.txt
```

Both must be empty (no uncommitted changes) before proceeding. If either shows output, STOP - do not use `git checkout --` for cleanup in Step 3 below (it would discard whatever uncommitted change already exists there); instead plan to remove the added bench-target lines by hand once the exact lines you added are known.

### Step 1: Write the throwaway bench

Create a temporary `tests/bench_move_face_commit.cpp` (add a temporary target to `tests/CMakeLists.txt`, same pattern the memory file describes for the original `bench_move_face.cpp` - a plain `main()`, not a gtest binary) that:

1. Builds the same N×N-hole plate fixture used for the original 25/100/225/400-hole measurements (pitch 15mm, r 3mm - match `movefaceop-freeze.md`'s table exactly so the numbers are comparable).
2. Simulates the full gesture: run a `MoveFacePreviewJob` (Task 2) to completion (as `MoveFaceController` would during a drag/keystroke), capture its `shape`/`key`.
3. Build a fresh `MoveFaceOp` (as `commitMoveFace` would), call `setPrecomputedResult(base, previewResult.shape, previewResult.key)`, then `execute()` on the LIVE-equivalent document, and time just that `execute()` call.
4. Print the time for 25/100/225/400 holes, same table shape as the memory file.

### Step 2: Run it and record the result

```bash
cmake --build build --target bench_move_face_commit -j8
./build/tests/bench_move_face_commit
```

Report the actual numbers in this plan's final summary - do not assume a number in advance. Compare against the existing 14967.6 ms figure at 400 holes (already measured, in `movefaceop-freeze.md`, do not re-derive that baseline).

### Step 3: Revert the throwaway bench

Given Step 0 confirmed `tests/CMakeLists.txt` was clean before you touched it, `git checkout --` now only discards YOUR OWN addition - safe:

```bash
git status --porcelain tests/CMakeLists.txt   # confirm it shows exactly your addition, nothing else
git checkout -- tests/CMakeLists.txt
rm tests/bench_move_face_commit.cpp
```

If Step 0 found the file already dirty, do not run `git checkout --` here - instead open the file and remove only the lines you added for this bench target, leaving the pre-existing changes untouched.

(Matches this project's established convention for throwaway benches, per the memory file's own note that the original `bench_move_face.cpp` was "reverted after measuring.")

### Step 4: Update the project memory

Edit `~/.claude/projects/-Users-laptop-Documents-Coding-projects-Materialzr/memory/movefaceop-freeze.md`: change the "RESIDUAL, not yet fixed" section to a "FIXED" note with this plan's commit(s) and the measured before/after numbers from Step 2. Update the `~/.claude/projects/-Users-laptop-Documents-Coding-projects-Materialzr/memory/MEMORY.md` index line for `movefaceop-freeze` to say fully fixed.

### Declined findings (round 1 Codex review)

Two findings from the Codex plan review were evaluated and declined rather than acted on:

1. **"`abandon()` doesn't cancel the worker, so a Confirm-mid-flight fallback runs concurrently with the abandoned worker, contending for CPU - the plan's 'exactly today's cost' claim ignores this."** True, but this concurrent-worker characteristic already exists in the CURRENTLY SHIPPED code: today, EVERY commit unconditionally calls `m_mfJob.abandon()` then unconditionally runs `MoveFaceOp::execute()` synchronously - the exact same contention scenario, on every single commit, not just the rare fallback case. This plan does not introduce it and does not make it worse; adoption makes it fire LESS often (only when the landed key doesn't match current params), never more. Building real cooperative cancellation into `AsyncJob` would be a separate, larger, cross-cutting change (it is used by Shell/Taper/ScaleFace/PushPull too, not just Move Face) and is out of scope for this bounded follow-up. The plan's wording was corrected (see the comment added in Task 3 Step 4) from "exactly today's cost" to state the comparison precisely: bounded by today's cost, and strictly less frequent.

2. **"The adopted path skips `BRepCheck_Analyzer`/solid-count/shell-count/volume-ratio validation - `setPrecomputedResult()` can bypass all of it."** True, but this is an inherent property of the ALREADY-SHIPPED, ALREADY-REVIEWED `Operation::Precomputed`/`canAdopt` mechanism that `ShellOp`/`TaperOp`/`ScaleFaceOp` already use identically (confirmed by reading `ShellOp::execute()`'s adopt branch: it also calls `doc.updateBody(...); return true;` immediately, skipping whatever validation its own recompute path does) - not something specific to or newly introduced by Move Face. The safety argument is structural, not per-op: `canAdopt` only fires when the WORKER's own `execute()` (which runs the identical validity guards, including Move Face's `BRepCheck_Analyzer`/solid-count/shell-count/volume-ratio checks at lines 472-518) already returned `ok=true` on that exact shape. Re-running those guards again at commit time would be checking the same shape against the same guards a second time for zero new information - and since Move Face's guards are themselves part of the expensive path (they run on the RESULT after the costly loft/cut), re-checking them would defeat a meaningful fraction of the performance win this plan exists to deliver. Fixing this "properly" would mean re-architecting the shared mechanism across four operations, far outside this plan's bound.

### Step 5: Final report

Summarize for the user: what changed (3 commits), the measured Confirm-click cost before/after at 400 holes, full test suite status (113/113 or updated count), and whether Task 3 Step 5's controller-level test was added or skipped (and why, if skipped).
