#pragma once
#include <string>

namespace materializr {

class MaterialLibrary;
class ShapeRenderer;

class MaterialPanel {
public:
    MaterialPanel();

    void setMaterialLibrary(MaterialLibrary* lib);
    void setShapeRenderer(ShapeRenderer* renderer);

    // Render. Returns true if material assignment changed.
    bool render();

    // Currently selected material index for assignment
    int getSelectedMaterial() const;

private:
    MaterialLibrary* m_library = nullptr;
    ShapeRenderer* m_renderer = nullptr;
    int m_selectedMaterial = 0;

    // Custom material editor
    bool m_editingCustom = false;
    char m_customName[64] = "Custom";
    float m_customColor[3] = {0.7f, 0.7f, 0.7f};
    float m_customRoughness = 0.5f;
    float m_customMetallic = 0.0f;
};

} // namespace materializr
