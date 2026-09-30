#pragma once
#include <string>
#include <vector>
#include <TopoDS_Shape.hxx>
#include "StepIO.h" // For ImportResult and ExportResult

class Document;

namespace materializr {

class IgesIO {
public:
    // Import an IGES file, adding all shapes as bodies to the document
    static ImportResult import(const std::string& filePath, Document& doc);

    // Export all visible bodies from the document to an IGES file
    static ExportResult exportFile(const std::string& filePath, const Document& doc);
};

} // namespace materializr
