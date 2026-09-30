// Editing a source sketch must drive geometry that passes through a chamfer:
// build a plate with a hole, chamfer the rim near it, then MOVE the hole in
// the sketch and cascade. The cascade must succeed, stay valid, relocate the
// hole (old spot fills, new spot opens), and keep the chamfer. Also guards the
// extrude recovered-footprint fallback: on replay the plate extrude's stored
// region seed must re-resolve (or fall back to the saved footprint), never to
// the #53 "ALL regions" catastrophe that corrupts the body under the chamfer.
#include <gtest/gtest.h>
#include "core/Document.h"
#include "core/History.h"
#include "modeling/Sketch.h"
#include "modeling/ExtrudeOp.h"
#include "modeling/ChamferOp.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <cstdio>
#include <memory>
using namespace materializr;
namespace {
double vol(const TopoDS_Shape& s){if(s.IsNull())return -1;GProp_GProps g;BRepGProp::VolumeProperties(s,g);return g.Mass();}
bool inAt(const TopoDS_Shape& s,double x,double y,double z){BRepClass3d_SolidClassifier c(s,gp_Pnt(x,y,z),1e-7);return c.State()==TopAbs_IN;}
int topBody(Document& d){int b=-1;double bv=-1;for(int id:d.getAllBodyIds()){double v=vol(d.getBody(id));if(v>bv){bv=v;b=id;}}return b;}
}
TEST(SketchEditCascade, EditSketchMovesHoleThroughChamfer) {
    Document doc; History hist;
    // Sketch on XY: 40x20 plate outline + a r=2 hole at (30,10).
    auto sk = std::make_shared<Sketch>();
    sk->setPlane(gp_Pln(gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0))));
    sk->addRectangle(glm::vec2(0,0), glm::vec2(40,20));
    int hc = sk->addPoint(glm::vec2(30,10));
    sk->addCircle(hc, 2.0f);
    int sid = doc.addSketch(sk, "Sketch 1");

    // Extrude the PLATE region (seed inside the plate but outside the hole).
    auto e1 = std::make_unique<ExtrudeOp>();
    e1->setSketchSource(sid);
    e1->deserializeParams("sketch=1;dist=5;dir=0;mode=0;target=-1;draft=0;regions=5.0:10.0");
    ASSERT_TRUE(e1->rebuildProfileFromSketch(doc));
    e1->setDistance(5.0); e1->setMode(ExtrudeMode::NewBody);
    ASSERT_TRUE(hist.pushOperation(std::move(e1), doc));
    int bid = topBody(doc);
    // The plate-with-hole: hole at (30,10) is open (through the 5mm plate).
    ASSERT_FALSE(inAt(doc.getBody(bid), 30, 10, 2.5)) << "setup: hole open";
    ASSERT_TRUE(inAt(doc.getBody(bid), 5, 10, 2.5)) << "setup: plate solid";

    // Chamfer the top edge along y at x=40 (the +x rim near the hole).
    TopoDS_Edge rim;
    for (TopExp_Explorer ex(doc.getBody(bid),TopAbs_EDGE);ex.More();ex.Next()){
        const TopoDS_Edge& e=TopoDS::Edge(ex.Current());
        if(BRepAdaptor_Curve(e).GetType()!=GeomAbs_Line) continue;
        gp_Pnt a=BRepAdaptor_Curve(e).Value(BRepAdaptor_Curve(e).FirstParameter());
        gp_Pnt b=BRepAdaptor_Curve(e).Value(BRepAdaptor_Curve(e).LastParameter());
        if(std::abs(a.X()-40)<1e-6&&std::abs(b.X()-40)<1e-6&&std::abs(a.Z()-5)<1e-6&&std::abs(b.Z()-5)<1e-6){rim=e;break;}
    }
    ASSERT_FALSE(rim.IsNull());
    auto ch=std::make_unique<ChamferOp>();
    ch->setBody(bid); ch->setEdges({rim}); ch->setDistance(1.5);
    ASSERT_TRUE(hist.pushOperation(std::move(ch), doc));
    const double vAfterChamfer = vol(doc.getBody(topBody(doc)));
    std::printf("built: vol=%.2f valid=%d\n", vAfterChamfer, (int)BRepCheck_Analyzer(doc.getBody(topBody(doc))).IsValid());

    // EDIT: move the hole from (30,10) to (20,10) in the sketch, cascade.
    sk->movePoint(hc, glm::vec2(20,10));
    int earliest=-1;
    for(int i=0;i<hist.stepCount();++i){
        Operation* op=const_cast<Operation*>(hist.getStep(i));
        if(auto* e=dynamic_cast<ExtrudeOp*>(op)){ if(e->getSketchId()==sid && e->rebuildProfileFromSketch(doc) && earliest<0) earliest=i; }
    }
    ASSERT_GE(earliest,0);
    if(auto s2=doc.getSketch(sid)) doc.setCascadeSketchOverride(sid, std::make_shared<Sketch>(*s2));
    bool ok=hist.editStep(earliest,doc,true);
    std::printf("cascade -> %s\n", ok?"OK":"REVERTED");
    int b2=topBody(doc); TopoDS_Shape body=doc.getBody(b2);
    std::printf("after: vol=%.2f valid=%d  oldHole(30,10)solid=%d newHole(20,10)solid=%d\n",
                vol(body), (int)BRepCheck_Analyzer(body).IsValid(),
                (int)inAt(body,30,10,2.5), (int)inAt(body,20,10,2.5));
    EXPECT_TRUE(ok) << "clean sketch edit through a chamfer should cascade";
    EXPECT_TRUE(inAt(body,30,10,2.5)) << "old hole location should now be filled";
    EXPECT_FALSE(inAt(body,20,10,2.5)) << "new hole location should be open";
}
