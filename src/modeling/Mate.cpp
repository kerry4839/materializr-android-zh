#include "Mate.h"
#include "../core/Document.h"

namespace materializr {

bool bodyIsMatePlaced(const Document& doc, int bodyId) {
    // Defense-in-depth: not currently reachable (createMate/ProjectIO's
    // loader both only ever populate a valid bodyB), but this is called
    // directly from TransformOp::execute/BatchTransformOp::execute on a
    // caller-supplied id that could in principle be uninitialized (-1).
    if (bodyId < 0) return false;
    for (const auto& m : doc.getMates()) {
        // A BROKEN mate still owns its body: letting a move through means the
        // solve silently undoes it once the anchors resolve again.
        if (m.suppressed) continue;
        if (m.bodyB == bodyId) return true;
    }
    return false;
}

} // namespace materializr
