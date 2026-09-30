// Recover a project whose fillet body snapshot got corrupted by a phantom
// re-fillet, while its fillet PARAMETERS stayed correct. Re-executes the fillet
// cleanly from its saved params against the pre-fillet body and writes a
// repaired copy (never overwrites the original).
//
// Usage: probe_repair <project> <out>

#include "core/Document.h"
#include "core/Operation.h"
#include "io/ProjectIO.h"
#include "modeling/FilletOp.h"

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cstdio>
#include <map>

using materializr::ProjectIO;
using materializr::ProjectHistory;

static int faceCount(const TopoDS_Shape& b) {
    int n = 0; for (TopExp_Explorer e(b, TopAbs_FACE); e.More(); e.Next()) ++n; return n;
}

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: %s <project> <out>\n", argv[0]); return 2; }
    Document doc;
    ProjectHistory hist;
    if (!ProjectIO::load(argv[1], doc, &hist).success) { std::fprintf(stderr, "load failed\n"); return 1; }

    std::map<int, TopoDS_Shape> running;
    for (auto& [id, s] : hist.initialState) running[id] = s;

    bool fixed = false;
    for (auto& st : hist.steps) {
        if (st.typeId == "fillet" && !fixed) {
            int bid = st.changed.empty() ? -1 : st.changed.front().first;
            if (bid < 0 || !running.count(bid)) { std::fprintf(stderr, "no pre-fillet body\n"); return 1; }
            TopoDS_Shape preFillet = running[bid];

            Operation::ReloadState reload;
            reload.modifiedBefore.push_back({bid, preFillet});
            reload.modifiedAfter.push_back({bid, st.changed.front().second});

            FilletOp op;
            if (!op.deserializeParams(st.params) || !op.rehydrateFromReload(reload, doc)) {
                std::fprintf(stderr, "fillet reconstruct failed\n"); return 1;
            }
            doc.updateBody(bid, preFillet);
            if (!op.execute(doc)) { std::fprintf(stderr, "clean fillet execute failed\n"); return 1; }
            TopoDS_Shape clean = doc.getBody(bid);
            std::printf("fillet repaired: %d -> %d faces\n",
                        faceCount(st.changed.front().second), faceCount(clean));

            // Rewrite this step's snapshot AND every later snapshot of the same
            // body (there are none that change it, but be safe) to the clean body.
            st.changed.front().second = clean;
            fixed = true;
        }
        for (auto& [id, shp] : st.changed) running[id] = shp;
        for (int id : st.deleted) running.erase(id);
    }
    if (!fixed) { std::fprintf(stderr, "no fillet step to repair\n"); return 1; }

    // doc's live bodies now reflect the clean body; write the repaired project.
    for (auto& [id, shp] : running) doc.updateBody(id, shp);
    auto res = ProjectIO::save(argv[2], doc, &hist);
    if (!res.success) { std::fprintf(stderr, "save failed: %s\n", res.errorMessage.c_str()); return 1; }
    std::printf("wrote repaired project: %s\n", argv[2]);
    return 0;
}
