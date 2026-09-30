#pragma once
#include <string>

namespace materializr {

class Sketch;

struct SvgExportResult {
    bool success = false;
    std::string errorMessage;
    int curveCount = 0;
};

class SvgExport {
public:
    // Export a sketch's non-construction geometry to a 1:1-millimeter SVG,
    // intended for laser cutters / 2.5D CNC. Curves are polyline-approximated
    // (lines exact; circles/arcs/splines tessellated). Y is flipped to SVG's
    // top-left origin so the part isn't mirrored. Construction geometry is
    // skipped (reference-only, not cut).
    static SvgExportResult exportSketch(const std::string& filePath, const Sketch& sketch);
};

} // namespace materializr
