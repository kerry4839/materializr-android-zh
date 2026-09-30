#pragma once
#include <string>
#include <vector>
#include <TopoDS_Shape.hxx>
#include <glm/glm.hpp>

class Document;

namespace materializr {

struct GltfExportResult {
    bool success = false;
    std::string errorMessage;
    int meshCount = 0;
};

class GltfExport {
public:
    static GltfExportResult exportFile(const std::string& filePath, const Document& doc);
};

} // namespace materializr
