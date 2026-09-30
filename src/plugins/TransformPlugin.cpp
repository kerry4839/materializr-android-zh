#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"

// Rotate/Scale are now interactive gizmos driven from the Transform group at the
// top of the body tools (Move / Rotate / Scale), so the old fixed-amount
// "Rotate 45°" / "Scale 1.5x" buttons were removed. TransformOp still backs the
// gizmo commits and the scale panel.
REGISTER_PLUGIN(Transform, [](materializr::PluginContext& /*ctx*/) {})
