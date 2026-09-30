#include "LiveOpPreview.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../core/Operation.h"

namespace materializr {

void LiveOpPreview::hold(std::unique_ptr<Operation> newOp, Document& doc) {
    clear(doc);
    m_op = std::move(newOp);
}

void LiveOpPreview::retract(Document& doc) {
    if (!m_applied || !m_op) return;
    // Swallow: an op that throws on undo has already left the document in
    // whatever state it managed, and there is nothing better to do from here
    // than stop treating the preview as applied.
    try { m_op->undo(doc); } catch (...) {}
    m_applied = false;
}

bool LiveOpPreview::apply(Document& doc) {
    if (!m_op) return false;
    try {
        m_applied = m_op->execute(doc);
    } catch (...) {
        m_applied = false;
    }
    return m_applied;
}

void LiveOpPreview::clear(Document& doc) {
    retract(doc);
    m_op.reset();
}

bool LiveOpPreview::commit(History& hist, Document& doc) {
    if (!m_applied || !m_op) { m_op.reset(); m_applied = false; return false; }
    // Document-taking overload: this is a live drag commit, and a mated body
    // must follow its reference the moment the drag lands, not at the next
    // unrelated push.
    hist.pushExecuted(std::move(m_op), doc);
    m_op.reset();
    m_applied = false;
    return true;
}

} // namespace materializr
