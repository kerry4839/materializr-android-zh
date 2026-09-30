#pragma once

namespace materializr {

class AboutDialog {
public:
    AboutDialog();
    void setVisible(bool vis);
    bool isVisible() const;
    void render();
private:
    bool m_visible = false;
};

} // namespace materializr
