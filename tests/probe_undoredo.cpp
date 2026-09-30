// Repro: undo/redo of a sketch edit on a sketch-driven FILLETED body re-runs
// the cascade through the fillet. Does a phantom default-radius (R1) fillet
// creep in across undo/redo cycles? Mirrors Application's Ctrl+Z/Ctrl+Y +
// cascadeFromSketchEdit path on a synthetic box.

#include "core/Document.h"
#include "core/History.h"
#include "modeling/Sketch.h"
#include "modeling/SketchEditOp.h"
#include "modeling/ExtrudeOp.h"
#include "modeling/FilletOp.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax3.hxx>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

using materializr::Sketch;
using materializr::SketchEditOp;

static void dumpRadii(const char* tag, const TopoDS_Shape& b) {
    int total = 0; std::map<int,int> radii;
    for (TopExp_Explorer e(b, TopAbs_FACE); e.More(); e.Next(), ++total) {
        try {
            BRepAdaptor_Surface s(TopoDS::Face(e.Current()));
            if (s.GetType() == GeomAbs_Cylinder) radii[(int)(s.Cylinder().Radius()*10+0.5)]++;
        } catch (...) {}
    }
    std::printf("%-22s %d faces  ", tag, total);
    for (auto& kv : radii) std::printf("[R%.1f x%d] ", kv.first/10.0, kv.second);
    std::printf("\n");
}

static std::shared_ptr<Sketch> makeRect(double w, double h, int pid[4]) {
    auto sk = std::make_shared<Sketch>();
    sk->setPlane(gp_Pln(gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0))));
    pid[0]=sk->addPoint({0,0}); pid[1]=sk->addPoint({(float)w,0});
    pid[2]=sk->addPoint({(float)w,(float)h}); pid[3]=sk->addPoint({0,(float)h});
    sk->addLine(pid[0],pid[1]); sk->addLine(pid[1],pid[2]);
    sk->addLine(pid[2],pid[3]); sk->addLine(pid[3],pid[0]);
    return sk;
}

int main() {
    Document doc; History hist;
    int pid[4];
    auto sk = makeRect(20, 10, pid);
    int sid = doc.addSketch(sk);

    auto ext = std::make_unique<ExtrudeOp>();
    ExtrudeOp* extP = ext.get();
    extP->setSketchSource(sid); extP->setDistance(10.0);
    extP->rebuildProfileFromSketch(doc); extP->execute(doc);
    hist.pushExecuted(std::move(ext));
    int body = doc.getAllBodyIds().front();

    // Fillet all four vertical corners, R3, anchored to the sketch.
    std::vector<TopoDS_Edge> verts;
    for (TopExp_Explorer ex(doc.getBody(body), TopAbs_EDGE); ex.More(); ex.Next()) {
        BRepAdaptor_Curve c(TopoDS::Edge(ex.Current()));
        if (c.GetType()==GeomAbs_Line && std::abs(c.Line().Direction().Dot(gp_Dir(0,0,1)))>0.999)
            verts.push_back(TopoDS::Edge(ex.Current()));
    }
    auto fil = std::make_unique<FilletOp>();
    fil->setBody(body); fil->setEdges(verts); fil->setRadius(3.0); fil->setSourceSketch(sid);
    fil->execute(doc);
    hist.pushExecuted(std::move(fil));
    dumpRadii("after fillet", doc.getBody(body));

    // A "Modify sketch" edit: shrink width 20 -> 16.
    auto before = std::make_shared<Sketch>(*sk);
    sk->movePoint(pid[1], {16,0}); sk->movePoint(pid[2], {16,10});
    auto after = std::make_shared<Sketch>(*sk);
    hist.pushExecuted(std::make_unique<SketchEditOp>(sk, before, after));

    // cascadeFromSketchEdit equivalent: re-derive the extrude profile, pin the
    // sketch's final state, replay from the extrude.
    auto cascade = [&]() {
        extP->rebuildProfileFromSketch(doc);
        doc.setCascadeSketchOverride(sid, std::make_shared<Sketch>(*sk));
        hist.editStep(0, doc, /*transactional=*/true);
        doc.clearCascadeSketchOverrides();
    };
    cascade();
    dumpRadii("after edit+cascade", doc.getBody(body));

    // Now the reported trigger: undo/redo cycles, each followed by a cascade
    // (as Application's Ctrl+Z / Ctrl+Y do while in sketch mode).
    for (int i = 0; i < 4; ++i) {
        hist.undo(doc); cascade();
        dumpRadii("after UNDO+cascade", doc.getBody(body));
        hist.redo(doc); cascade();
        dumpRadii("after REDO+cascade", doc.getBody(body));
    }
    return 0;
}
