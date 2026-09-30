#include "../plugin/PluginMacro.h"
#include "../core/Document.h"
#include "../io/StlExport.h"
#include "../io/FileDialogs.h"
#include <cstdio>

REGISTER_PLUGIN(StlExport, [](materializr::PluginContext& ctx) {
    ctx.registerIOFormat({"STL", {"stl"}, false, true,
        nullptr,
        [](materializr::PluginContext& ctx, const std::string&) {
            materializr::FileDialogs::exportFile("Export STL", "export.stl",
                "application/octet-stream",
                {{"STL Files", "*.stl"}},
                [&ctx](const std::string& path) {
                    return materializr::StlExport::exportFile(path, ctx.document()).success;
                });
            return true;
        },
        // Export a caller-supplied document (the "export just these
        // bodies" path hands over a scratch doc holding copies).
        [](const Document& doc, const std::string& path) {
            return materializr::StlExport::exportFile(path, doc).success;
        }});
})
