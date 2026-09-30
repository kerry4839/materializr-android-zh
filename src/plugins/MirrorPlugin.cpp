#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"

// Mirror is now driven by the Application's own toolbar button + popup (one
// "Mirror" button → choose axis X/Y/Z, or pick a face to mirror across), so the
// per-axis plugin toolbar buttons were removed. MirrorOp itself still does the
// work; see Application's renderMirrorPopup() / face-pick handling.
REGISTER_PLUGIN(Mirror, [](materializr::PluginContext& /*ctx*/) {})
