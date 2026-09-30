#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../core/SelectionManager.h"
#include "../modeling/DeleteOp.h"
#include <vector>

REGISTER_PLUGIN(Delete, [](materializr::PluginContext& ctx) {
    ctx.registerToolbarButton({"Delete", "Edit",
        materializr::SelectionContext::HasBodies, 900,
        [](materializr::PluginContext& ctx) {
            const auto& sel = ctx.selection().getSelection();
            std::vector<int> bodiesToDelete;
            for (const auto& entry : sel) {
                if (entry.bodyId >= 0) {
                    bool already = false;
                    for (int b : bodiesToDelete) { if (b == entry.bodyId) { already = true; break; } }
                    if (!already) bodiesToDelete.push_back(entry.bodyId);
                }
            }
            for (int bodyId : bodiesToDelete) {
                auto op = std::make_unique<DeleteOp>();
                op->setBodyId(bodyId);
                ctx.history().pushOperation(std::move(op), ctx.document());
            }
            ctx.selection().clear();
            ctx.markMeshesDirty();
        }, nullptr,
        "Delete the selected body / bodies. Undoable from the History panel."});

    ctx.registerCommand({"Delete Selected", "Delete",
        [](materializr::PluginContext& ctx) {
            const auto& sel = ctx.selection().getSelection();
            std::vector<int> bodiesToDelete;
            for (const auto& entry : sel) {
                if (entry.bodyId >= 0) {
                    bool already = false;
                    for (int b : bodiesToDelete) { if (b == entry.bodyId) { already = true; break; } }
                    if (!already) bodiesToDelete.push_back(entry.bodyId);
                }
            }
            for (int bodyId : bodiesToDelete) {
                auto op = std::make_unique<DeleteOp>();
                op->setBodyId(bodyId);
                ctx.history().pushOperation(std::move(op), ctx.document());
            }
            ctx.selection().clear();
            ctx.markMeshesDirty();
        }});
})
