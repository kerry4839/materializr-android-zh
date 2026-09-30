#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"

// Shell is now the Application's interactive face op (popup + live preview +
// editable thickness). The toolbar "Shell" button lives in the app's
// renderFaceTools, not here.
REGISTER_PLUGIN(Shell, [](materializr::PluginContext& /*ctx*/) {
})
