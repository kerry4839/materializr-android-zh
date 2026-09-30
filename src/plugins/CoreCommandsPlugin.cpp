#include "../plugin/PluginMacro.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../viewport/Camera.h"

REGISTER_PLUGIN(CoreCommands, [](materializr::PluginContext& ctx) {
    ctx.registerCommand({"Undo", "Ctrl+Z", [](materializr::PluginContext& ctx) {
        if (ctx.history().canUndo()) ctx.history().undo(ctx.document());
        ctx.markMeshesDirty();
    }});

    ctx.registerCommand({"Redo", "Ctrl+Y", [](materializr::PluginContext& ctx) {
        if (ctx.history().canRedo()) ctx.history().redo(ctx.document());
        ctx.markMeshesDirty();
    }});

    ctx.registerCommand({"Reset Camera", "Home", [](materializr::PluginContext& ctx) {
        const_cast<materializr::Camera&>(ctx.camera()).reset();
    }});
})
