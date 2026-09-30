#include "../plugin/PluginMacro.h"
#include "../core/Document.h"
#include "../io/GltfExport.h"
#include "../io/FileDialogs.h"
#include <cstdio>

REGISTER_PLUGIN(GltfExport, [](materializr::PluginContext& ctx) {
    ctx.registerIOFormat({"glTF", {"glb", "gltf"}, false, true,
        nullptr,
        [](materializr::PluginContext& ctx, const std::string&) {
            materializr::FileDialogs::exportFile("Export glTF", "export.glb",
                "application/octet-stream",
                {{"glTF Binary", "*.glb"}, {"glTF", "*.gltf"}},
                [&ctx](const std::string& path) {
                    return materializr::GltfExport::exportFile(path, ctx.document()).success;
                });
            return true;
        },
        // Export a caller-supplied document (the "export just these
        // bodies" path hands over a scratch doc holding copies).
        [](const Document& doc, const std::string& path) {
            return materializr::GltfExport::exportFile(path, doc).success;
        }});
})
