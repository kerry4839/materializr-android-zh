// Repro: does re-executing the fillet (as a history rebuild / cascade does)
// stack a phantom default-radius fillet, doubling the body's faces? Loads a
// project, reconstructs the fillet op from its saved params against the
// pre-fillet body, executes it once and then again, reporting rounded-face
// radii each time.
//
// Usage: probe_rebuild <project>

#include "core/Document.h"
#include "core/Operation.h"
#include "io/ProjectIO.h"
#include "modeling/FilletOp.h"

#include <BRepAdaptor_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cstdio>
#include <map>

using materializr::ProjectIO;
using materializr::ProjectHistory;

static void report(const char* tag, const TopoDS_Shape& b) {
    int total = 0; std::map<int,int> radii;
    for (TopExp_Explorer e(b, TopAbs_FACE); e.More(); e.Next(), ++total) {
        try {
            BRepAdaptor_Surface s(TopoDS::Face(e.Current()));
            if (s.GetType() == GeomAbs_Cylinder) radii[(int)(s.Cylinder().Radius()*10+0.5)]++;
            else if (s.GetType() == GeomAbs_Torus) radii[(int)(s.Torus().MinorRadius()*10+0.5)]++;
        } catch (...) {}
    }
    std::printf("%s: %d faces  ", tag, total);
    for (auto& kv : radii) std::printf("[R%.1f x%d] ", kv.first/10.0, kv.second);
    std::printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: %s <project>\n", argv[0]); return 2; }
    Document doc;
    ProjectHistory hist;
    if (!ProjectIO::load(argv[1], doc, &hist).success) { std::fprintf(stderr, "load failed\n"); return 1; }

    // Walk history: accumulate bodies, find the fillet step + its pre-state.
    std::map<int, TopoDS_Shape> running;
    for (auto& [id, s] : hist.initialState) running[id] = s;
    for (const auto& st : hist.steps) {
        if (st.typeId == "fillet") {
            Operation::ReloadState reload;
            int bid = -1;
            for (auto& [id, shp] : st.changed) {
                if (running.count(id)) {
                    reload.modifiedBefore.push_back({id, running[id]});
                    reload.modifiedAfter.push_back({id, shp});
                    bid = id;
                }
            }
            if (bid < 0) { std::printf("fillet step has no modified body\n"); return 1; }
            TopoDS_Shape preFillet = running[bid];
            report("pre-fillet (base)", preFillet);
            report("saved fillet result", st.changed.front().second);

            FilletOp op;
            if (!op.deserializeParams(st.params)) { std::printf("deser failed\n"); return 1; }
            std::printf("params radius=%.3f\n", op.getRadius());
            if (!op.rehydrateFromReload(reload, doc)) { std::printf("rehydrate failed\n"); return 1; }

            // Drive the doc body to the pre-fillet state and execute the fillet
            // the way a rebuild replay does.
            doc.updateBody(bid, preFillet);
            bool ok1 = op.execute(doc);
            std::printf("execute #1: %s\n", ok1 ? "ok" : "FAIL");
            if (ok1) report("  after execute #1", doc.getBody(bid));

            // Second execute WITHOUT rolling back (stacking test).
            bool ok2 = op.execute(doc);
            std::printf("execute #2 (no rollback): %s\n", ok2 ? "ok" : "FAIL");
            if (ok2) report("  after execute #2", doc.getBody(bid));

            // Re-execute the PROPER way: reset body to pre-fillet, execute again.
            doc.updateBody(bid, preFillet);
            bool ok3 = op.execute(doc);
            std::printf("execute #3 (rolled back first): %s\n", ok3 ? "ok" : "FAIL");
            if (ok3) report("  after execute #3", doc.getBody(bid));
            return 0;
        }
        for (auto& [id, shp] : st.changed) running[id] = shp;
        for (int id : st.deleted) running.erase(id);
    }
    std::printf("no fillet step found\n");
    return 0;
}
