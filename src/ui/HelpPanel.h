#pragma once

namespace materializr {

// A docked panel that walks new users through the basics: navigation, sketches,
// modelling, history. Toggled from Help → User Guide.
class HelpPanel {
public:
    HelpPanel() = default;

    // `v == true` also raises the window: an already-open guide buried under
    // a full-screen window (the home page) must surface on re-request, not
    // silently stay behind it.
    void setVisible(bool v) { m_visible = v; m_raise = v; }
    void toggle()           { m_visible = !m_visible; }
    bool isVisible() const  { return m_visible; }

    void render();

private:
    bool m_visible = false;
    bool m_raise = false;   // one-shot focus on the next render
};

} // namespace materializr
