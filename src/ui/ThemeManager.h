#pragma once

namespace materializr {

enum class Theme { Dark, Light };

class ThemeManager {
public:
    ThemeManager();

    void setTheme(Theme theme);
    Theme getTheme() const;
    void toggle();

    // Apply the current theme to ImGui
    void apply();

    // Render theme selector (for menu bar)
    // Returns true if theme changed
    bool renderSelector();

private:
    Theme m_theme = Theme::Dark;

    void applyDark();
    void applyLight();
};

} // namespace materializr
