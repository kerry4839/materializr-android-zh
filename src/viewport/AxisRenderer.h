#pragma once
#include "gl_common.h"
#include <glm/glm.hpp>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <vector>
#include <string>

namespace materializr {

// Renders construction axes as a coloured line segment of length
// ±halfLength along the axis direction, with a small origin marker
// (cross-tick) so the user can see the anchor point. Mirrors
// PlaneRenderer's shape so the ConstructionAxisPlugin can register a
// matching render pass.
class AxisRenderer {
public:
    AxisRenderer();
    ~AxisRenderer();

    bool initialize();

    struct AxisData {
        gp_Pnt origin;
        gp_Dir direction;
        std::string name;
        glm::vec4 color;
        float halfLength;
        bool visible;
        bool selected = false;
    };

    void addAxis(const gp_Pnt& origin, const gp_Dir& direction,
                 const std::string& name,
                 glm::vec4 color = glm::vec4(0.95f, 0.55f, 0.20f, 1.0f),
                 float halfLength = 50.0f,
                 bool selected = false);

    void render(const glm::mat4& view, const glm::mat4& projection);
    void clear();

private:
    bool compileShader(unsigned int& shader, unsigned int type, const char* source);
    bool linkProgram(unsigned int vertShader, unsigned int fragShader);

    std::vector<AxisData> m_axes;
    unsigned int m_program = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    int m_locMVP = -1;
    int m_locColor = -1;
};

} // namespace materializr
