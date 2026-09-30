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

TopoDS_Face bottomFace(const TopoDS_Shape& s) {
    TopoDS_Face best; double bestZ = 1e300;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() < bestZ) { bestZ = g.CentreOfMass().Z(); best = TopoDS::Face(e.Current()); }
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

TEST(MoveFaceAsync, CommitAdoptsTheLandedPreviewForAReversedFace) {
    Harness h;
    TopoDS_Shape body = makeHolePlate(5); // 25 holes
    int bodyId = h.doc.addBody(body, "plate");
    TopoDS_Face face = bottomFace(h.doc.getBody(bodyId));
    ASSERT_EQ(face.Orientation(), TopAbs_REVERSED)
        << "fixture assumption broken - the bottom face of makeHolePlate is not REVERSED";

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

    const TopoDS_Shape landed = h.doc.getBody(bodyId);
    ASSERT_FALSE(landed.IsNull());
    ASSERT_FALSE(landed.IsEqual(body))
        << "preview never actually applied to the body (pre-existing reversed-face bug)";
    const int adoptedBefore = MoveFaceOp::adoptedCount();
    const int recomputedBefore = MoveFaceOp::recomputedCount();

    mfc.commitMoveFace(ctx);
    const TopoDS_Shape committed = h.doc.getBody(bodyId);
    EXPECT_TRUE(committed.IsEqual(landed))
        << "commit did not adopt the landed preview for a reversed face - it recomputed instead";
    EXPECT_EQ(MoveFaceOp::adoptedCount(), adoptedBefore + 1)
        << "adoption did not fire for a reversed target face";
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
