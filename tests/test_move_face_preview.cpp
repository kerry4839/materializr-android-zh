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
