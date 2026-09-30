#include "../plugin/PluginMacro.h"
#include "../plugin/PluginContext.h"

// Face → Extrude was redundant with Push/Pull on a face, so the toolbar
// button + "Extrude Face" palette command are gone. The only Extrude path
// left is sketch → new body, which lives on the sketch-selected toolbar
// (ToolAction::ExtrudeSketch → Application::extrudeSketchById) and always
// runs in NewBody mode under our current model:
//   - Push/Pull → modify the body the sketch / face is on
//   - Extrude   → make a separate body
// REGISTER_PLUGIN keeps the forceLink_Extrude symbol alive so ForceLink.cpp
// links cleanly.
REGISTER_PLUGIN(Extrude, [](materializr::PluginContext& /*ctx*/) {
})
