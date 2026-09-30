#include "../plugin/PluginMacro.h"
#include "../core/Document.h"
#include "../io/ObjExport.h"
#include "../io/FileDialogs.h"

REGISTER_PLUGIN(ObjExport, [](materializr::PluginContext& ctx) {
    ctx.registerIOFormat({"OBJ", {"obj"}, false, true,
        nullptr,
        [](materializr::PluginContext& ctx, const std::string&) {
            materializr::FileDialogs::exportFile("Export OBJ", "export.obj",
                "application/octet-stream",
                {{"OBJ Files", "*.obj"}},
                [&ctx](const std::string& path) {
                    return materializr::ObjExport::exportFile(path, ctx.document()).success;
                });
            return true;
        },
        // Export a caller-supplied document (the "export just these
        // bodies" path hands over a scratch doc holding copies).
        [](const Document& doc, const std::string& path) {
            return materializr::ObjExport::exportFile(path, doc).success;
        }});
})
