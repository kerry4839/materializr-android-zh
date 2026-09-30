#include "MaterialPanel.h"
#include "../core/Material.h"
#include "../viewport/ShapeRenderer.h"

#include <imgui.h>
#include <cstring>
#include "../i18n.h"
#include "../i18n.h"

namespace materializr {

MaterialPanel::MaterialPanel()
{
    std::memset(m_customName, 0, sizeof(m_customName));
    std::strncpy(m_customName, "Custom", sizeof(m_customName) - 1);
}

void MaterialPanel::setMaterialLibrary(MaterialLibrary* lib)
{
    m_library = lib;
}

void MaterialPanel::setShapeRenderer(ShapeRenderer* renderer)
{
    m_renderer = renderer;
}

int MaterialPanel::getSelectedMaterial() const
{
    return m_selectedMaterial;
}

bool MaterialPanel::render()
{
    if (!m_library) return false;

    bool changed = false;

    ImGui::Begin(materializr::tr("Materials###MZMaterials"));

    // Material preset list
    ImGui::Text("%s", materializr::tr("Presets"));
    ImGui::Separator();

    const auto& materials = m_library->getAll();
    for (int i = 0; i < static_cast<int>(materials.size()); ++i) {
        const Material& mat = materials[i];

        ImVec4 color(mat.baseColor.x, mat.baseColor.y, mat.baseColor.z, 1.0f);
        ImGui::PushID(i);

        // Color preview square
        if (ImGui::ColorButton("##color", color, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20))) {
            m_selectedMaterial = i;
        }

        ImGui::SameLine();

        bool isSelected = (m_selectedMaterial == i);
        if (ImGui::Selectable(materializr::tr(mat.name.c_str()), isSelected)) {
            m_selectedMaterial = i;
        }

        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Show properties of selected material
    if (m_selectedMaterial >= 0 && m_selectedMaterial < static_cast<int>(materials.size())) {
        const Material& sel = materials[m_selectedMaterial];
        ImGui::Text(materializr::tr("Selected: %s"), sel.name.c_str());
        ImGui::Text(materializr::tr("Roughness: %.2f"), sel.roughness);
        ImGui::Text(materializr::tr("Metallic: %.2f"), sel.metallic);
        if (sel.transmission > 0.0f) {
            ImGui::Text(materializr::tr("Transmission: %.2f"), sel.transmission);
            ImGui::Text(materializr::tr("IOR: %.2f"), sel.ior);
        }
    }

    ImGui::Spacing();

    // Assign to selected button
    if (ImGui::Button(materializr::tr("Assign to Selected"))) {
        changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Custom material editor
    if (ImGui::CollapsingHeader(materializr::tr("Custom Material###Custom Material"))) {
        m_editingCustom = true;

        ImGui::InputText(materializr::tr("Name"), m_customName, sizeof(m_customName));
        ImGui::ColorEdit3("Base Color", m_customColor);
        ImGui::SliderFloat(materializr::tr("Roughness"), &m_customRoughness, 0.0f, 1.0f);
        ImGui::SliderFloat(materializr::tr("Metallic"), &m_customMetallic, 0.0f, 1.0f);

        if (ImGui::Button(materializr::tr("Add Custom Material"))) {
            Material custom;
            custom.name = m_customName;
            custom.baseColor = glm::vec3(m_customColor[0], m_customColor[1], m_customColor[2]);
            custom.roughness = m_customRoughness;
            custom.metallic = m_customMetallic;
            m_selectedMaterial = m_library->addCustom(custom);
        }
    }

    ImGui::End();

    return changed;
}

} // namespace materializr
