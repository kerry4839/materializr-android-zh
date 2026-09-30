#include "../plugin/PluginMacro.h"
#include "../core/Document.h"
#include "../io/StlIO.h"
#include <cstdio>

// STL import. The menu entry registers an importable "STL" IO format; because
// the import needs an options dialog (accuracy + wireframe + file) that only the
// Application can render, the importFn just asks the Application to open that
// dialog via requestInteractiveOp. The Application drains the request, shows the
// dialog, and runs StlIO::import on commit. (Export lives in StlExportPlugin.)
REGISTER_PLUGIN(StlImport, [](materializr::PluginContext& ctx) {
    ctx.registerIOFormat({"STL", {"stl"}, /*canImport=*/true, /*canExport=*/false,
        [](materializr::PluginContext& ctx, const std::string&) {
            ctx.requestInteractiveOp(materializr::InteractiveOp::StlImport);
            return true;
        },
        nullptr});
})
