#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"
#include "../core/Document.h"
#include "../core/History.h"
#include "../core/SelectionManager.h"
#include "../modeling/PatternOp.h"
#include <cstdio>

REGISTER_PLUGIN(Pattern, [](materializr::PluginContext& ctx) {
    // Both buttons hand off to Application's interactive popup, which lets the
    // user pick axis / count / spacing (linear) or axis / count / angle / origin
    // (radial) with a live preview, then confirm or cancel. The actual op is
    // pushed onto history at confirm time.
    ctx.registerToolbarButton({"Linear Pattern", "Pattern",
        materializr::SelectionContext::HasBodies, 300,
        [](materializr::PluginContext& ctx) {
            ctx.requestInteractiveOp(materializr::InteractiveOp::LinearPattern);
        }, nullptr,
        "Open a popup to copy the selected body along an axis a fixed distance "
        "apart. Pick X / Y / Z, count, and spacing with a live preview."});

    ctx.registerToolbarButton({"Circular Pattern", "Pattern",
        materializr::SelectionContext::HasBodies, 301,
        [](materializr::PluginContext& ctx) {
            ctx.requestInteractiveOp(materializr::InteractiveOp::RadialPattern);
        }, nullptr,
        "Open a popup to copy the selected body around an axis. Pick the axis "
        "(X / Y / Z), count, total angle, and click in the viewport to pick the "
        "axis origin with grid snapping."});
})
