#include "ToastNotification.h"
#include <imgui.h>
#include <algorithm>

namespace materializr {

ToastNotification::ToastNotification() = default;

void ToastNotification::show(const std::string& message, float durationSec) {
    m_toasts.push_back({message, durationSec});
}

void ToastNotification::render() {
    if (m_toasts.empty()) return;

    float dt = ImGui::GetIO().DeltaTime;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    float padding = 16.0f;
    float toastHeight = 36.0f;
    float toastSpacing = 6.0f;
    float maxToastWidth = 350.0f;

    // Draw toasts from bottom-right, stacking upwards
    float yOffset = padding;

    for (int i = static_cast<int>(m_toasts.size()) - 1; i >= 0; --i) {
        Toast& toast = m_toasts[i];
        toast.remainingTime -= dt;

        // Compute alpha for fade-out in the last 0.5 seconds
        float alpha = 1.0f;
        if (toast.remainingTime < 0.5f) {
            alpha = toast.remainingTime / 0.5f;
            if (alpha < 0.0f) alpha = 0.0f;
        }

        // Calculate text size to determine toast width
        ImVec2 textSize = ImGui::CalcTextSize(toast.message.c_str());
        float toastWidth = textSize.x + 24.0f;
        if (toastWidth > maxToastWidth) toastWidth = maxToastWidth;

        float xPos = displaySize.x - toastWidth - padding;
        float yPos = displaySize.y - yOffset - toastHeight;

        ImGui::SetNextWindowPos(ImVec2(xPos, yPos));
        ImGui::SetNextWindowSize(ImVec2(toastWidth, toastHeight));
        ImGui::SetNextWindowBgAlpha(0.85f * alpha);

        char windowName[64];
        std::snprintf(windowName, sizeof(windowName), "##Toast_%d", i);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin(windowName, nullptr, flags)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
            ImGui::TextUnformatted(toast.message.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        yOffset += toastHeight + toastSpacing;
    }

    // Remove expired toasts
    m_toasts.erase(
        std::remove_if(m_toasts.begin(), m_toasts.end(),
            [](const Toast& t) { return t.remainingTime <= 0.0f; }),
        m_toasts.end());
}

} // namespace materializr
