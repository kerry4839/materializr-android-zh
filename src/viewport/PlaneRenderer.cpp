#include "PlaneRenderer.h"

#include <glm/gtc/type_ptr.hpp>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <cstdio>

namespace materializr {

static const char* s_planeVertSource = R"(
#version 330 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_mvp;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

static const char* s_planeFragSource = R"(
#version 330 core

uniform vec4 u_color;

out vec4 fragColor;

void main() {
    fragColor = u_color;
}
)";

PlaneRenderer::PlaneRenderer() = default;

PlaneRenderer::~PlaneRenderer() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}

bool PlaneRenderer::initialize() {
    // Compile shaders
    unsigned int vertShader = 0, fragShader = 0;
    if (!compileShader(vertShader, GL_VERTEX_SHADER, s_planeVertSource)) {
        return false;
    }
    if (!compileShader(fragShader, GL_FRAGMENT_SHADER, s_planeFragSource)) {
        glDeleteShader(vertShader);
        return false;
    }
    if (!linkProgram(vertShader, fragShader)) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return false;
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    // Cache uniform locations
    m_locMVP = glGetUniformLocation(m_program, "u_mvp");
    m_locColor = glGetUniformLocation(m_program, "u_color");

    // Create VAO and VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void PlaneRenderer::addPlane(const gp_Pln& plane, const std::string& name,
                              glm::vec4 color, float size, bool selected) {
    PlaneData data;
    data.plane = plane;
    data.name = name;
    data.color = color;
    data.size = size;
    data.visible = true;
    data.selected = selected;
    m_planes.push_back(std::move(data));
}

void PlaneRenderer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!m_program || m_planes.empty()) return;

    glm::mat4 vp = projection * view;

    glUseProgram(m_program);

    // Enable blending for transparent planes
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writes to avoid z-fighting, but keep depth test
    glDepthMask(GL_FALSE);

    glBindVertexArray(m_vao);

    for (const auto& pd : m_planes) {
        if (!pd.visible) continue;

        // Get plane coordinate system from OCCT
        const gp_Ax1& axis = pd.plane.Axis();
        gp_Pnt origin = axis.Location();
        gp_Dir normal = axis.Direction();

        // Get the X and Y directions of the plane
        gp_Dir xDir = pd.plane.Position().XDirection();
        gp_Dir yDir = pd.plane.Position().YDirection();

        float s = pd.size;

        // Origin in GLM
        glm::vec3 o(static_cast<float>(origin.X()),
                     static_cast<float>(origin.Y()),
                     static_cast<float>(origin.Z()));
        glm::vec3 dx(static_cast<float>(xDir.X()),
                      static_cast<float>(xDir.Y()),
                      static_cast<float>(xDir.Z()));
        glm::vec3 dy(static_cast<float>(yDir.X()),
                      static_cast<float>(yDir.Y()),
                      static_cast<float>(yDir.Z()));

        // Compute 4 corners: origin +/- size * xDir +/- size * yDir
        glm::vec3 c0 = o - s * dx - s * dy;
        glm::vec3 c1 = o + s * dx - s * dy;
        glm::vec3 c2 = o + s * dx + s * dy;
        glm::vec3 c3 = o - s * dx + s * dy;

        // Two triangles for the quad (6 vertices)
        float quadVerts[] = {
            c0.x, c0.y, c0.z,
            c1.x, c1.y, c1.z,
            c2.x, c2.y, c2.z,

            c0.x, c0.y, c0.z,
            c2.x, c2.y, c2.z,
            c3.x, c3.y, c3.z,
        };

        // Upload quad data
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_DYNAMIC_DRAW);

        // Selected planes get a brighter fill + amber border at full
        // opacity so the user knows the click landed without us having to
        // pop the gizmo on top.
        glm::vec4 fillColor   = pd.color;
        glm::vec4 borderColor;
        float     borderWidth;
        if (pd.selected) {
            fillColor   = glm::vec4(0.95f, 0.75f, 0.20f, 0.30f); // warm amber tint
            borderColor = glm::vec4(1.00f, 0.78f, 0.20f, 1.00f);
            borderWidth = 3.0f;
        } else {
            borderColor = glm::vec4(pd.color.r, pd.color.g, pd.color.b,
                                    glm::min(pd.color.a * 4.0f, 0.8f));
            borderWidth = 1.5f;
        }

        // Draw the filled quad
        glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(vp));
        glUniform4fv(m_locColor, 1, glm::value_ptr(fillColor));

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw the border as a line loop
        float borderVerts[] = {
            c0.x, c0.y, c0.z,
            c1.x, c1.y, c1.z,
            c2.x, c2.y, c2.z,
            c3.x, c3.y, c3.z,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(borderVerts), borderVerts, GL_DYNAMIC_DRAW);
        glUniform4fv(m_locColor, 1, glm::value_ptr(borderColor));

        glLineWidth(borderWidth);
        glDrawArrays(GL_LINE_LOOP, 0, 4);

        // Normal indicator: a short stalk from the plane centre along +normal
        // so the facing direction is visible (and Flip Normal has a visible
        // effect). Length scales with the plane half-size.
        glm::vec3 nrm(static_cast<float>(normal.X()),
                      static_cast<float>(normal.Y()),
                      static_cast<float>(normal.Z()));
        glm::vec3 nTip = o + (s * 0.5f) * nrm;
        float normalVerts[] = {
            o.x,    o.y,    o.z,
            nTip.x, nTip.y, nTip.z,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(normalVerts), normalVerts, GL_DYNAMIC_DRAW);
        glUniform4fv(m_locColor, 1, glm::value_ptr(borderColor));
        glLineWidth(borderWidth);
        glDrawArrays(GL_LINES, 0, 2);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Restore depth writes
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void PlaneRenderer::clear() {
    m_planes.clear();
}

bool PlaneRenderer::compileShader(unsigned int& shader, unsigned int type,
                                   const char* source) {
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::fprintf(stderr, "PlaneRenderer shader compilation failed: %s\n", infoLog);
        glDeleteShader(shader);
        shader = 0;
        return false;
    }
    return true;
}

bool PlaneRenderer::linkProgram(unsigned int vertShader, unsigned int fragShader) {
    m_program = glCreateProgram();
    glAttachShader(m_program, vertShader);
    glAttachShader(m_program, fragShader);
    glLinkProgram(m_program);

    int success = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::fprintf(stderr, "PlaneRenderer shader linking failed: %s\n", infoLog);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

} // namespace materializr
