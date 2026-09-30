#pragma once
#include <glm/glm.hpp>
#include <TopoDS_Shape.hxx>
#include <string>
#include <vector>

class Document;
class SelectionManager;

namespace materializr {

// What the user wants to measure. Set when they click Measure and pick a mode.
enum class MeasureMode {
    Inactive,    // tool is off
    PickMode,    // tool active, showing the mode-picker buttons
    Object,      // bbox dimensions of the currently-selected body/bodies
    Edge,        // sum of curve lengths of the currently-selected edges
    Line         // two-click distance, drawn as a purple line in the viewport
};

struct MeasureResult {
    enum Type { None, Distance, EdgeLength, FaceArea, Angle, BoundingBox };
    Type type = None;
    double value = 0.0;
    std::string label;
    glm::vec3 pointA{0};
    glm::vec3 pointB{0};
    // Extra values for bounding box (width, height, depth)
    double dimX = 0.0;
    double dimY = 0.0;
    double dimZ = 0.0;
};

class MeasureTool {
public:
    MeasureTool();

    void setDocument(const Document* doc);
    void setSelectionManager(const SelectionManager* sel);

    // Mode controls.
    MeasureMode getMode() const { return m_mode; }
    void setMode(MeasureMode m);

    // Rebuild m_results based on the current mode + state. Called every frame
    // (cheap when nothing's selected) so the panel stays in sync.
    void update();

    // Render the measurement results / mode picker panel.
    void renderPanel();

    // Point-to-point inputs. Each click in the viewport (while the tool is
    // in PointToPoint mode) reports a captured world position here.
    void capturePoint(glm::vec3 p);
    void resetPointCapture();
    int getCapturedPointCount() const { return m_pointsCaptured; }
    glm::vec3 getCapturedPoint(int i) const { return (i == 0) ? m_point1 : m_point2; }

    // Get results for viewport overlay rendering
    const std::vector<MeasureResult>& getResults() const;

    void clear();
    bool isActive() const { return m_mode != MeasureMode::Inactive; }

private:
    const Document* m_document = nullptr;
    const SelectionManager* m_selection = nullptr;
    std::vector<MeasureResult> m_results;

    MeasureMode m_mode = MeasureMode::Inactive;
    // Point-to-point capture state.
    glm::vec3 m_point1{0.0f};
    glm::vec3 m_point2{0.0f};
    int       m_pointsCaptured = 0; // 0, 1, or 2

    void measureObjects();        // Object mode: combined bbox of bodies
    void measureEdges();          // Edge   mode: sum of selected edge lengths
    void measureLine();   // PointToPoint mode: distance between picks
};

} // namespace materializr
