#pragma once
#include <string>
#include <vector>
#include <TopoDS_Shape.hxx>

class Document;

namespace materializr {

struct ImportResult {
    bool success = false;
    std::string errorMessage;
    int bodiesImported = 0;
    // STL only: triangle counts before/after decimation and final B-rep face
    // count (0 when not applicable). Used for the import-result toast.
    int trianglesBefore = 0;
    int trianglesAfter = 0;
    int faceCount = 0;
};

struct ExportResult {
    bool success = false;
    std::string errorMessage;
};

class StepIO {
public:
    // Import a STEP file, adding all shapes as bodies to the document
    static ImportResult import(const std::string& filePath, Document& doc);

    // Export all visible bodies from the document to a STEP file
    static ExportResult exportFile(const std::string& filePath, const Document& doc);

    // Export specific bodies by ID
    static ExportResult exportBodies(const std::string& filePath, const Document& doc,
                                     const std::vector<int>& bodyIds);
};

} // namespace materializr
