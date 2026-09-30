#pragma once

// History-step mutations shared by HistoryPanel and PropertiesPanel, pulled
// out of the ImGui render code so a unit test can exercise the same logic
// the panels call. Each wraps its History call in a BodyChangeScope, since
// undo/redo/edit/enable/remove can touch an unknown-ahead-of-time set of
// bodies (base bodies an op modifies, split results, cascades, etc).

#include "Document.h"
#include "History.h"
#include "BodyChanges.h"

#include <functional>

namespace materializr {

inline bool undoStep(History& history, Document& doc,
                      const std::function<void(int)>& markBodyDirty) {
    BodyChangeScope scope(doc, markBodyDirty);
    return history.undo(doc);
}

inline bool redoStep(History& history, Document& doc,
                      const std::function<void(int)>& markBodyDirty) {
    BodyChangeScope scope(doc, markBodyDirty);
    return history.redo(doc);
}

// In-place enable/disable toggle - preserves base bodies the op modifies
// (a full replayAll's doc.clear() would delete them).
inline bool toggleStepEnabled(History& history, Document& doc, int index, bool enabled,
                               const std::function<void(int)>& markBodyDirty) {
    BodyChangeScope scope(doc, markBodyDirty);
    return history.setStepEnabled(index, enabled, doc);
}

inline bool deleteStep(History& history, Document& doc, int index,
                        const std::function<void(int)>& markBodyDirty) {
    BodyChangeScope scope(doc, markBodyDirty);
    return history.removeStep(index, doc);
}

// Transactional: a failed replay restores the model wholesale rather than
// leaving it half-built.
inline bool applyStepEdit(History& history, Document& doc, int index,
                           const std::function<void(int)>& markBodyDirty) {
    BodyChangeScope scope(doc, markBodyDirty);
    return history.editStep(index, doc, /*transactional=*/true);
}

} // namespace materializr
