#include "MeshGuard.h"

#include "Document.h"
#include "SelectionManager.h"
#include "../i18n.h"

namespace materializr {

std::vector<int> meshBodiesAmong(const Document& doc,
                                 const std::vector<int>& bodies) {
    std::vector<int> meshes;
    for (int id : bodies)
        if (id >= 0 && doc.isBodyMesh(id)) meshes.push_back(id);
    return meshes;
}

std::vector<int> selectedBodyIds(const SelectionManager& sel) {
    std::vector<int> bodies;
    for (const auto& e : sel.getSelection()) {
        if (e.bodyId < 0) continue;
        bool seen = false;
        for (int b : bodies) if (b == e.bodyId) { seen = true; break; }
        if (!seen) bodies.push_back(e.bodyId);
    }
    return bodies;
}

std::string meshRefusalMessage(const char* opName, size_t meshCount,
                               size_t total) {
    const char* op = opName ? opName : "That operation";
    // An import is a reference, so say what it IS good for in the same breath -
    // otherwise the refusal reads as a missing feature rather than a boundary.
    if (meshCount >= total) {
        return materializr::trFormat("%s needs solid geometry, and this is an "
                                     "imported mesh - a reference body. Sketch "
                                     "on it and snap to it all you like, then "
                                     "model the part alongside it.",
                                     materializr::tr(op));
    }
    return materializr::trFormat("%s needs solid geometry, and one of the "
                                 "selected bodies is an imported mesh - a "
                                 "reference body. Leave the import out of the "
                                 "selection.", materializr::tr(op));
}

} // namespace materializr
