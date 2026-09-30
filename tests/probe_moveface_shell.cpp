// PROBE: does MoveFaceOp on a shelled body keep the cavity?
#include "modeling/ShellOp.h"
#include "modeling/MoveFaceOp.h"
#include "core/Document.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cstdio>
#include <cmath>

static double vol(const TopoDS_Shape& s){GProp_GProps g;BRepGProp::VolumeProperties(s,g);return g.Mass();}
static TopoDS_Face faceWithNormal(const TopoDS_Shape& b, double nx,double ny,double nz){
    for (TopExp_Explorer ex(b,TopAbs_FACE);ex.More();ex.Next()){
        TopoDS_Face f=TopoDS::Face(ex.Current());
        BRepGProp_Face p(f);double u1,u2,v1,v2;p.Bounds(u1,u2,v1,v2);
        gp_Pnt c;gp_Vec n;p.Normal(.5*(u1+u2),.5*(v1+v2),c,n);
        if(n.Magnitude()<1e-9)continue;n.Normalize();
        if(n.X()*nx+n.Y()*ny+n.Z()*nz>0.99)return f;
    }
    return {};
}
int main(){
    Document doc;
    int body=doc.addBody(BRepPrimAPI_MakeBox(gp_Pnt(0,0,0),20,10,10).Shape(),"box");
    printf("solid vol %.1f\n", vol(doc.getBody(body)));
    ShellOp sh; sh.setBody(body); sh.setThickness(1.0);
    sh.addFaceToRemove(faceWithNormal(doc.getBody(body),0,0,1)); // open top
    if(!sh.execute(doc)){printf("shell FAILED\n");return 1;}
    double hollowV=vol(doc.getBody(body));
    printf("shelled vol %.1f (hollow)\n",hollowV);
    // Now MoveFace-slide the +X wall by +2 in-plane... slide is in-plane; for a
    // side wall (+X normal), in-plane = Y/Z. Slide it +2 in Y.
    MoveFaceOp mv; mv.setBody(body);
    mv.setFace(faceWithNormal(doc.getBody(body),1,0,0));
    mv.setKind(MoveFaceOp::Kind::Translate);
    mv.setMoveVector(gp_Vec(0,2,0));
    bool ok=mv.execute(doc);
    double after=vol(doc.getBody(body));
    printf("moveface(slide +X wall +2Y) ok=%d vol %.1f -> cavity %s\n",
           ok?1:0, after, (after<hollowV*1.5)?"KEPT":"LOST (re-solidified)");
    // And a rotate (taper) of the +X wall:
    Document doc2;
    int b2=doc2.addBody(BRepPrimAPI_MakeBox(gp_Pnt(0,0,0),20,10,10).Shape(),"box2");
    ShellOp sh2; sh2.setBody(b2); sh2.setThickness(1.0);
    sh2.addFaceToRemove(faceWithNormal(doc2.getBody(b2),0,0,1));
    sh2.execute(doc2);
    double hollow2=vol(doc2.getBody(b2));
    MoveFaceOp mv2; mv2.setBody(b2);
    mv2.setFace(faceWithNormal(doc2.getBody(b2),1,0,0));
    mv2.setKind(MoveFaceOp::Kind::Rotate);
    mv2.setRotation(gp_Dir(0,1,0),0.17); // ~10 deg
    bool ok2=mv2.execute(doc2);
    double after2=vol(doc2.getBody(b2));
    printf("moveface(rotate +X wall 10deg) ok=%d vol %.1f (hollow was %.1f) -> cavity %s\n",
           ok2?1:0, after2, hollow2, (after2<hollow2*1.5)?"KEPT":"LOST (re-solidified)");
    return 0;
}
