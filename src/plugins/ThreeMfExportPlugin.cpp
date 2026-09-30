#include "../plugin/PluginMacro.h"
#include "../core/Document.h"
#include "../io/ThreeMfExport.h"
#include "../io/FileDialogs.h"

REGISTER_PLUGIN(ThreeMfExport, [](materializr::PluginContext& ctx) {
    ctx.registerIOFormat({"3MF", {"3mf"}, false, true,
        nullptr,
        [](materializr::PluginContext& ctx, const std::string&) {
            materializr::FileDialogs::exportFile("Export 3MF", "export.3mf",
                "model/3mf",
                {{"3MF Files", "*.3mf"}},
                [&ctx](const std::string& path) {
                    return materializr::ThreeMfExport::exportFile(path, ctx.document()).success;
                });
            return true;
        },
        // Export a caller-supplied document (the "export just these
        // bodies" path hands over a scratch doc holding copies).
        [](const Document& doc, const std::string& path) {
            return materializr::ThreeMfExport::exportFile(path, doc).success;
        }});
})
