// Tail: PushPullOp face-driven targets via topo::Ref. Push a solid's face, then
// an upstream sketch edit rebuilds & MOVES that face; on replay the op
// re-resolves the profile face by its sketch-anchored name and pushes it again,
// instead of using the stale handle (old body) and pushing nothing.

#include "modeling/TopoName.h"
#include "modeling/PushPullOp.h"
#include "core/Document.h"
#include "modeling/Sketch.h"
#include "modeling/ExtrudeOp.h"

#include <gtest/gtest.h>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <memory>

using materializr::Sketch;
using namespace materializr;

namespace {

std::shared_ptr<Sketch> makeRect(double w, double h, int pid[4]) {
    auto sk = std::make_shared<Sketch>();
    sk->setPlane(gp_Pln(gp_Ax3(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0))));
    pid[0] = sk->addPoint({0.0f, 0.0f});
    pid[1] = sk->addPoint({(float)w, 0.0f});
    pid[2] = sk->addPoint({(float)w, (float)h});
    pid[3] = sk->addPoint({0.0f, (float)h});
    sk->addLine(pid[0], pid[1]);
    sk->addLine(pid[1], pid[2]);
    sk->addLine(pid[2], pid[3]);
    sk->addLine(pid[3], pid[0]);
    return sk;
}

TopoDS_Face topCap(const TopoDS_Shape& body) {
    TopoDS_Face best; double bestZ = -1e18;
    for (TopExp_Explorer ex(body, TopAbs_FACE); ex.More(); ex.Next()) {
        TopoDS_Face f = TopoDS::Face(ex.Current());
        GProp_GProps g; BRepGProp::SurfaceProperties(f, g);
        if (g.CentreOfMass().Z() > bestZ) { bestZ = g.CentreOfMass().Z(); best = f; }
    }
    return best;
}

void bbox(const TopoDS_Shape& s, double& xmax, double& zmax) {
    Bnd_Box b; BRepBndLib::Add(s, b);
    double x0, y0, z0, x1, y1, z1; b.Get(x0, y0, z0, x1, y1, z1);
    xmax = x1; zmax = z1;
}

} // namespace

TEST(TopoPushPull, FaceTargetFollowsResize) {
    Document doc;
    int pid[4];
    auto sk = makeRect(20.0, 10.0, pid);
    int sid = doc.addSketch(sk);
    ExtrudeOp ext; ext.setSketchSource(sid); ext.setDistance(10.0);
    ASSERT_TRUE(ext.rebuildProfileFromSketch(doc));
    ASSERT_TRUE(ext.execute(doc));
    int body = doc.getAllBodyIds().front();

    // Push the TOP face (+Z) OUT by 5 -> the box grows to z=15.
    PushPullOp pp;
    PushPullOp::Target t;
    t.profile = topCap(doc.getBody(body));
    t.sourceBodyId = body;
    pp.setTargets({ t });
    pp.setDistance(5.0);
    ASSERT_TRUE(pp.execute(doc));
    double xmax, zmax; bbox(doc.getBody(body), xmax, zmax);
    EXPECT_NEAR(zmax, 15.0, 1e-6) << "top pushed out to z=15";

    // WIDEN the sketch 20 -> 40: the base rebuilds and the top face relocates
    // (now a 40-wide cap). Re-derive the base, then replay the push/pull.
    sk->movePoint(pid[1], {40.0f, 0.0f});
    sk->movePoint(pid[2], {40.0f, 10.0f});
    ASSERT_TRUE(ext.rebuildProfileFromSketch(doc));
    ASSERT_TRUE(ext.execute(doc));
    bbox(doc.getBody(body), xmax, zmax);
    ASSERT_NEAR(xmax, 40.0, 1e-6) << "base widened to x=40";
    ASSERT_NEAR(zmax, 10.0, 1e-6) << "base back to z=10 before the re-push";

    // The push must re-resolve the relocated top cap and push it out again -> 15.
    ASSERT_TRUE(pp.execute(doc)) << "push/pull must re-resolve the moved face";
    bbox(doc.getBody(body), xmax, zmax);
    EXPECT_NEAR(xmax, 40.0, 1e-6) << "still the widened body";
    EXPECT_NEAR(zmax, 15.0, 1e-6)
        << "re-found top cap pushed to z=15 (not the stale handle -> no-op at 10)";
}
