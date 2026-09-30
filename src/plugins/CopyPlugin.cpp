#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../core/SelectionManager.h"
#include "../modeling/CopyOp.h"

// Duplicate-in-place action shared by the toolbar button and the Ctrl+D
// shortcut handler in Application. Pushes a CopyOp with zero offset, then
// re-selects the JUST-CREATED body so the user can immediately Move it.
// Two identical bodies stacked on top of each other are indistinguishable;
// landing on the new one (rather than leaving both selected) keeps the
// gizmo unambiguous and matches Ctrl+D / "duplicate in place" UX in other
// CAD tools.
namespace materializr { namespace dup_in_place {
inline void run(PluginContext& ctx) {
    const auto& sel = ctx.selection().getSelection();
    if (sel.empty() || sel[0].bodyId < 0) return;
    int srcId = sel[0].bodyId;
    int srcFolder = ctx.document().getBodyFolder(srcId);
    auto op = std::make_unique<CopyOp>();
    op->setSourceBodyId(srcId);
    op->setOffset(0.0, 0.0, 0.0);
    CopyOp* opPtr = op.get(); // op moves into history; raw pointer stays valid
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) return;
    int newId = opPtr->getCreatedBodyId();
    if (newId >= 0) {
        // Inherit the source's folder so the duplicate lands beside the
        // original in the Items panel instead of dropping to root.
        if (srcFolder >= 0) {
            ctx.document().setBodyFolder(newId, srcFolder);
        }
        SelectionEntry entry;
        entry.type = SelectionType::Body;
        entry.bodyId = newId;
        try { entry.shape = ctx.document().getBody(newId); } catch (...) {}
        ctx.selection().select(entry);
    }
    ctx.markMeshesDirty();
}
}} // namespace materializr::dup_in_place

REGISTER_PLUGIN(Copy, [](materializr::PluginContext& ctx) {
    ctx.registerToolbarButton({"Duplicate", "Edit",
        materializr::SelectionContext::HasBodies, 800,
        [](materializr::PluginContext& ctx) {
            materializr::dup_in_place::run(ctx);
        }, nullptr,
        "Add an exact copy of the selected body on top of itself. Use Move "
        "to reposition. Ctrl+D for keyboard."});
})
