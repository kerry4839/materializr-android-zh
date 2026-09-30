#pragma once
#include "Operation.h"
#include "Document.h"
#include "../modeling/SketchEditOp.h"
#include "../modeling/SketchTransformOp.h"
#include <functional>

namespace materializr {

// The sketch id `op`'s undo()/redo() drove via the sketch-edit cascade (a
// SketchEditOp/SketchTransformOp's own undo/redo only reverts sketch geometry;
// the body it drives is updated separately via SketchEditedEvent). -1 if `op`
// is null or isn't a sketch-mutating step.
inline int sketchIdForCascade(const Operation* op, const Document& doc) {
    if (!op) return -1;
    if (auto* se = dynamic_cast<const SketchEditOp*>(op)) {
        auto target = se->getTarget();
        return target ? doc.findSketchId(target.get()) : -1;
    }
    if (auto* st = dynamic_cast<const SketchTransformOp*>(op))
        return st->getSketchId();
    return -1;
}

// Fires `cascade(sketchId)` for every sketch that needs its driven body
// resynced after an undo()/redo() call, given:
// - `appliedIdx`: History::lastUndoneStep()/lastRedoneStep() from that call
//   (< 0 means nothing was actually applied - fires nothing, unconditionally)
// - `appliedOp`: history.getStep(appliedIdx) (null when appliedIdx < 0)
// - `doc`: for sketch-id resolution
// - `activeSketchId`: the in-progress sketch session's id, or -1 when not in
//   sketch mode / no active sketch (keeps an in-progress sketch's driven body
//   in sync even for an upstream op that isn't itself a sketch op)
// Fires each distinct sketch id at most once (the active sketch and the
// applied op's sketch are often the same id).
template <typename Cascade>
void dispatchUndoRedoCascade(int appliedIdx, const Operation* appliedOp,
                             const Document& doc, int activeSketchId,
                             Cascade&& cascade) {
    if (appliedIdx < 0) return; // nothing was actually applied - never cascade
    int cascaded = -1;
    if (activeSketchId >= 0) {
        cascade(activeSketchId);
        cascaded = activeSketchId;
    }
    if (int sid = sketchIdForCascade(appliedOp, doc); sid >= 0 && sid != cascaded)
        cascade(sid);
}

} // namespace materializr
