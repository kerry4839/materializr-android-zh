#pragma once
#include "gl_common.h"
#include <glm/glm.hpp>

namespace materializr {

class BackgroundRenderer {
public:
    BackgroundRenderer();
    ~BackgroundRenderer();

    bool initialize();
    void render();

    void setTopColor(glm::vec3 color);
    void setBottomColor(glm::vec3 color);

private:
    unsigned int m_program = 0;
    unsigned int m_vao = 0;
    int m_locTopColor = -1;
    int m_locBottomColor = -1;

    glm::vec3 m_topColor = glm::vec3(0.22f, 0.22f, 0.28f);
    glm::vec3 m_bottomColor = glm::vec3(0.12f, 0.12f, 0.15f);
};

} // namespace materializr
