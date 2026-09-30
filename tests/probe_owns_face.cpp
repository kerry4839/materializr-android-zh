// PROBE: reproduce "clicking a base-box face opens the Fillet editor" headless.
// Loads a project, rehydrates fillet/chamfer steps exactly like the app's
// reload loop, then asks ownsFace() about EVERY face of every final body.
// Usage: probe_owns_face <project.materializr>
#include "core/Document.h"
#include "io/ProjectIO.h"
#include "modeling/FilletOp.h"
#include "modeling/ChamferOp.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

static const char* surfName(GeomAbs_SurfaceType t) {
    switch (t) {
        case GeomAbs_Plane: return "plane";
        case GeomAbs_Cylinder: return "cylinder";
        case GeomAbs_Cone: return "cone";
        case GeomAbs_Sphere: return "sphere";
        case GeomAbs_Torus: return "torus";
        case GeomAbs_BSplineSurface: return "bspline";
        case GeomAbs_SurfaceOfExtrusion: return "extr";
        default: return "other";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: %s <project>\n", argv[0]); return 2; }
    Document doc;
    materializr::ProjectHistory hist;
    if (!materializr::ProjectIO::load(argv[1], doc, &hist).success) {
        std::printf("LOAD FAILED\n"); return 1;
    }
    std::printf("steps: %zu\n", hist.steps.size());

    std::map<int, TopoDS_Shape> running;
    for (const auto& [id, shp] : hist.initialState) running[id] = shp;

    struct Owner { std::string type; int step; std::unique_ptr<Operation> op; };
    std::vector<Owner> owners;
    int idx = 0;
    for (const auto& st : hist.steps) {
        Operation::ReloadState reload;
        for (const auto& [id, shape] : st.changed) {
            if (running.find(id) == running.end()) reload.created.push_back(id);
            else {
                reload.modifiedBefore.push_back(std::make_pair(id, running[id]));
                reload.modifiedAfter.push_back(std::make_pair(id, shape));
            }
        }
        for (const auto& [id, shape] : st.changed) running[id] = shape;
        if ((st.typeId == "fillet" || st.typeId == "chamfer") && !st.params.empty()) {
            std::unique_ptr<Operation> op;
            if (st.typeId == "fillet") op = std::make_unique<FilletOp>();
            else op = std::make_unique<ChamferOp>();
            bool des = op->deserializeParams(st.params);
            bool reh = des && op->rehydrateFromReload(reload, doc);
            std::printf("step %d [%s]: deserialize=%d rehydrate=%d\n",
                        idx, st.typeId.c_str(), des ? 1 : 0, reh ? 1 : 0);
            if (reh) {
                Owner ow;
                ow.type = st.typeId; ow.step = idx; ow.op = std::move(op);
                owners.push_back(std::move(ow));
            }
        }
        ++idx;
    }

    // Simulate the LIVE session: a sketch-edit cascade re-executes the fillet,
    // refreshing m_generatedFaces from the real builder. Roll the body back to
    // its pre-fillet state and execute the rehydrated op for real.
    for (auto& ow : owners) {
        auto* fil = dynamic_cast<FilletOp*>(ow.op.get());
        auto* cha = dynamic_cast<ChamferOp*>(ow.op.get());
        int bodyId = fil ? fil->getBodyId() : (cha ? cha->getBodyId() : -1);
        if (bodyId < 0) continue;
        // find the pre-step body state again
        std::map<int, TopoDS_Shape> pre;
        for (const auto& [id, shp] : hist.initialState) pre[id] = shp;
        for (int k = 0; k < ow.step; ++k)
            for (const auto& [id, shape] : hist.steps[k].changed) pre[id] = shape;
        if (pre.find(bodyId) == pre.end()) continue;
        doc.updateBody(bodyId, pre[bodyId]);
        bool ok = ow.op->execute(doc);
        std::printf("\nlive re-execute step %d [%s]: %s\n", ow.step,
                    ow.type.c_str(), ok ? "OK" : "FAILED");
        if (ok) running[bodyId] = doc.getBody(bodyId);
    }

    for (const auto& [bodyId, shape] : running) {
        int nf = 0, claimed = 0;
        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) ++nf;
        std::printf("\nbody %d (%d faces):\n", bodyId, nf);
        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
            TopoDS_Face f = TopoDS::Face(ex.Current());
            for (const auto& ow : owners) {
                if (!ow.op->ownsFace(f)) continue;
                ++claimed;
                BRepAdaptor_Surface s(f);
                GProp_GProps g; BRepGProp::SurfaceProperties(f, g);
                gp_Pnt c = g.CentreOfMass();
                std::printf("  CLAIMED by step %d [%s]: %s area %.1f "
                            "centre (%.1f, %.1f, %.1f)\n",
                            ow.step, ow.type.c_str(), surfName(s.GetType()),
                            g.Mass(), c.X(), c.Y(), c.Z());
            }
        }
        std::printf("  -> %d claims\n", claimed);
    }
    return 0;
}
