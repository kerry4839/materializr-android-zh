#pragma once

namespace materializr {

class ShortcutsPanel {
public:
    ShortcutsPanel();
    void setVisible(bool vis);
    bool isVisible() const;
    void render();
private:
    bool m_visible = false;
};

} // namespace materializr
