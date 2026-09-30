#pragma once
#include <glm/glm.hpp>

namespace materializr {

class SectionView;

class SectionPanel {
public:
    SectionPanel();

    void setSectionView(SectionView* sv);

    // Render the panel. Returns true if section needs update.
    bool render();

private:
    SectionView* m_sectionView = nullptr;
    int m_planeAxis = 1; // 0=X, 1=Y, 2=Z
    float m_offset = 0.0f;
};

} // namespace materializr
