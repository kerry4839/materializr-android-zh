#include "GenerationLedger.h"

#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepTools_History.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

namespace materializr {
namespace topo {

void GenerationLedger::capture(BRepBuilderAPI_MakeShape& mk,
                               const TopoDS_Shape& in, TopAbs_ShapeEnum t) {
    inputs.clear();
    generated.Clear();
    modified.Clear();
    captureAdd(mk, in, t);
}

void GenerationLedger::captureAdd(BRepBuilderAPI_MakeShape& mk,
                                  const TopoDS_Shape& in, TopAbs_ShapeEnum t) {
    if (in.IsNull()) return;
    inputs.push_back({in, t});

    TopTools_IndexedMapOfShape subs;
    TopExp::MapShapes(in, t, subs);
    for (int i = 1; i <= subs.Extent(); ++i) {
        const TopoDS_Shape& s = subs(i);
        try {
            const TopTools_ListOfShape& g = mk.Generated(s);
            if (!g.IsEmpty()) generated.Add(s, g);
        } catch (...) {}
        try {
            const TopTools_ListOfShape& m = mk.Modified(s);
            if (!m.IsEmpty()) modified.Add(s, m);
        } catch (...) {}
    }
}

int GenerationLedger::inputOf(const TopoDS_Shape& sub) const {
    for (size_t k = 0; k < inputs.size(); ++k) {
        TopTools_IndexedMapOfShape subs;
        TopExp::MapShapes(inputs[k].shape, inputs[k].type, subs);
        if (subs.Contains(sub)) return static_cast<int>(k);
    }
    return -1;
}

} // namespace topo
} // namespace materializr
