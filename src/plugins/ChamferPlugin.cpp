#include "../plugin/PluginMacro.h"

// Chamfer is implemented as the Application's interactive edge op (drag handle +
// live measurement + preview), which needs viewport drag input that the plugin
// InteractiveTool API does not yet receive. This registration is intentionally
// empty; the toolbar "Chamfer" button lives in the app's edge tools and routes
// through ToolAction::Chamfer -> Application::beginInteractiveEdgeOp.
//
// Migrating this back into a real plugin requires widening PluginContext to
// route ToolInputEvents to the active tool and to expose picker/preview state.
REGISTER_PLUGIN(Chamfer, [](materializr::PluginContext& /*ctx*/) {})
