#include "UiTheme.h"
#include "VariablePanel.h"
#include "../core/VariableManager.h"
#include <imgui.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <algorithm>
#include "../i18n.h"
#include "../i18n.h"

namespace materializr {

VariablePanel::VariablePanel() = default;

void VariablePanel::setVariableManager(VariableManager* mgr) {
    m_manager = mgr;
}

bool VariablePanel::render() {
    bool changed = false;

    ImGui::Begin(materializr::tr("Variables###MZVariables"));

    if (!m_manager) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", materializr::tr("No variable manager"));
        ImGui::End();
        return false;
    }

    const auto& vars = m_manager->getAll();

    // Build a sorted list of variable names for stable display order
    std::vector<std::string> names;
    names.reserve(vars.size());
    for (const auto& [name, var] : vars) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());

    // Table of existing variables
    if (!names.empty()) {
        if (ImGui::BeginTable("VarTable", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp)) {

            ImGui::TableSetupColumn(materializr::tr("Name"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(materializr::tr("Expression"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(materializr::tr("Value"), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("##Del", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableHeadersRow();

            int bufIdx = 0;
            for (const auto& name : names) {
                if (bufIdx >= 32) break;

                auto it = vars.find(name);
                if (it == vars.end()) continue;
                const Variable& var = it->second;

                ImGui::TableNextRow();

                // Name column
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", name.c_str());

                // Expression column (editable)
                ImGui::TableSetColumnIndex(1);

                // Initialize edit buffer if it doesn't match current expression
                // We detect a "fresh" buffer by checking if it's empty and expression is not
                if (m_editBuffers[bufIdx][0] == '\0' && !var.expression.empty()) {
                    std::strncpy(m_editBuffers[bufIdx], var.expression.c_str(),
                                 sizeof(m_editBuffers[bufIdx]) - 1);
                    m_editBuffers[bufIdx][sizeof(m_editBuffers[bufIdx]) - 1] = '\0';
                }

                char label[32];
                std::snprintf(label, sizeof(label), "##expr_%d", bufIdx);
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText(label, m_editBuffers[bufIdx],
                                     sizeof(m_editBuffers[bufIdx]),
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
                    m_manager->set(name, m_editBuffers[bufIdx]);
                    m_manager->recalculate();
                    changed = true;
                }

                // Value column
                ImGui::TableSetColumnIndex(2);
                if (var.valid) {
                    char valText[32];
                    std::snprintf(valText, sizeof(valText), "%.4g", var.value);
                    ImGui::Text("%s", valText);
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", materializr::tr("ERR"));
                    if (ImGui::IsItemHovered() && !var.error.empty()) {
                        ImGui::SetTooltip("%s", var.error.c_str());
                    }
                }

                // Delete button
                ImGui::TableSetColumnIndex(3);
                char delLabel[32];
                std::snprintf(delLabel, sizeof(delLabel), "X##del_%d", bufIdx);
                if (ImGui::SmallButton(delLabel)) {
                    m_manager->remove(name);
                    m_manager->recalculate();
                    // Clear this edit buffer
                    m_editBuffers[bufIdx][0] = '\0';
                    changed = true;
                }

                ++bufIdx;
            }

            ImGui::EndTable();
        }
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", materializr::tr("No variables defined"));
    }

    // Add Variable section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(materializr::accentText(), "%s", materializr::tr("Add Variable"));

    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputText("##NewName", m_newName, sizeof(m_newName));
    ImGui::SameLine();
    ImGui::Text("=");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-60.0f);
    ImGui::InputText("##NewExpr", m_newExpr, sizeof(m_newExpr));
    ImGui::SameLine();

    if (ImGui::Button(materializr::tr("Add"))) {
        if (m_newName[0] != '\0' && m_newExpr[0] != '\0') {
            if (m_manager->set(m_newName, m_newExpr)) {
                m_manager->recalculate();
                changed = true;
                m_newName[0] = '\0';
                m_newExpr[0] = '\0';
            }
        }
    }

    ImGui::End();
    return changed;
}

} // namespace materializr
