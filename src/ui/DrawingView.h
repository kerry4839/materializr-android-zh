#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <TopoDS_Shape.hxx>

class Document;

namespace materializr {

enum class ProjectionDir { Front, Back, Left, Right, Top, Bottom, Isometric };

struct DrawingLine {
    glm::vec2 start;
    glm::vec2 end;
    bool isHidden = false; // behind other geometry
    float weight = 1.0f;
};

struct DrawingViewData {
    ProjectionDir direction = ProjectionDir::Front;
    std::string name;
    glm::vec2 position{0}; // position on drawing sheet
    float scale = 1.0f;
    std::vector<DrawingLine> visibleLines;
    std::vector<DrawingLine> hiddenLines;
};

class DrawingView {
public:
    DrawingView();

    void setDocument(const Document* doc);

    // Generate projected views from the 3D model
    void generateViews();

    // Add a custom view direction
    void addView(ProjectionDir dir, glm::vec2 sheetPos, float scale = 1.0f);

    // Render the drawing workspace
    void render();

    // Export to DXF (simplified)
    bool exportDXF(const std::string& filePath);

    // Export to SVG
    bool exportSVG(const std::string& filePath);

private:
    const Document* m_document = nullptr;
    std::vector<DrawingViewData> m_views;
    float m_sheetWidth = 297.0f;  // A4 landscape
    float m_sheetHeight = 210.0f;

    void projectShape(const TopoDS_Shape& shape, ProjectionDir dir,
                      std::vector<DrawingLine>& visible,
                      std::vector<DrawingLine>& hidden);
    glm::vec2 projectPoint(const gp_Pnt& pt, ProjectionDir dir);
    void renderSheet();
    void renderView(const DrawingViewData& view);
};

} // namespace materializr
