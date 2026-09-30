#include "UiTheme.h"
#include "VersionPanel.h"
#include "../core/VersionManager.h"

#include <imgui.h>
#include <cstdio>
#include <ctime>
#include <cstring>
#include "../i18n.h"
#include "../i18n.h"
#include "../i18n.h"

namespace materializr {

VersionPanel::VersionPanel() {
    std::memset(m_labelBuffer, 0, sizeof(m_labelBuffer));
}

void VersionPanel::setVersionManager(VersionManager* mgr) {
    m_manager = mgr;
}

int VersionPanel::render() {
    int restoreId = -1;

    ImGui::Begin(materializr::tr("Versions###MZVersions"));

    if (!m_manager) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", materializr::tr("No version manager available."));
        ImGui::End();
        return -1;
    }

    // Save Version section
    ImGui::TextColored(materializr::accentText(), "%s", materializr::tr("Save Version"));
    ImGui::Separator();

    ImGui::SetNextItemWidth(-80.0f);
    ImGui::InputText("##label", m_labelBuffer, sizeof(m_labelBuffer));
    ImGui::SameLine();
    if (ImGui::Button(materializr::tr("Save"), ImVec2(-1, 0))) {
        // Save is handled externally since we need the Document;
        // we store the intent in restoreId as a special value
        // Actually, for saving we need a different mechanism.
        // For now, this button signals save via a convention:
        // The caller checks the label buffer. We use -2 as "save requested".
        restoreId = -2;
    }

    ImGui::Spacing();

    // Auto-save settings
    ImGui::TextColored(materializr::accentText(), "%s", materializr::tr("Auto-Save"));
    ImGui::Separator();

    int interval = m_manager->getAutoSaveInterval();
    int intervalMinutes = interval / 60;
    if (ImGui::SliderInt(materializr::tr("Interval (min)"), &intervalMinutes, 1, 30)) {
        m_manager->setAutoSaveInterval(intervalMinutes * 60);
    }

    if (m_manager->isAutoSaveDue()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", materializr::tr("Auto-save pending..."));
    } else {
        std::time_t now = std::time(nullptr);
        // Calculate approximate time since last potential auto-save
        int remaining = interval - static_cast<int>(now % interval);
        char timeBuf[64];
        std::snprintf(timeBuf, sizeof(timeBuf), materializr::tr("Next auto-save in ~%d:%02d"),
                      remaining / 60, remaining % 60);
        ImGui::Text("%s", timeBuf);
    }

    ImGui::Spacing();

    // Version list
    ImGui::TextColored(materializr::accentText(), "%s", materializr::tr("Version History"));
    ImGui::Separator();

    const auto& versions = m_manager->getVersions();

    if (versions.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", materializr::tr("No versions saved yet."));
    } else {
        ImGui::BeginChild("VersionList", ImVec2(0, 0), true);

        // Show versions in reverse order (newest first)
        for (int i = static_cast<int>(versions.size()) - 1; i >= 0; --i) {
            const auto& entry = versions[static_cast<size_t>(i)];

            ImGui::PushID(entry.id);

            // Format timestamp
            char timeBuf[64];
            struct tm* tm_info = std::localtime(&entry.timestamp);
            if (tm_info) {
                std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", tm_info);
            } else {
                // "%s" wrapper: tr() returns const char*, and a non-literal with no variadic
                // args trips -Werror=format-security even though it has no
                // specifier of its own.
                std::snprintf(timeBuf, sizeof(timeBuf), "%s", materializr::tr("(unknown time)"));
            }

            // Version entry display
            char headerBuf[256];
            std::snprintf(headerBuf, sizeof(headerBuf), "#%d: %s",
                          entry.id, entry.label.c_str());

            if (ImGui::TreeNode(headerBuf)) {
                ImGui::Text(materializr::tr("Date: %s"), timeBuf);

                char bodiesBuf[64];
                std::snprintf(bodiesBuf, sizeof(bodiesBuf), "Bodies: %d", entry.bodyCount);
                ImGui::Text("%s", bodiesBuf);

                if (ImGui::Button(materializr::tr("Restore"), ImVec2(-1, 0))) {
                    restoreId = entry.id;
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    ImGui::End();
    return restoreId;
}

} // namespace materializr
