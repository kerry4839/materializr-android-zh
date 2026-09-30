#include "gl_common.h"
#include "BackgroundRenderer.h"

#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

namespace materializr {

static const char* s_bgVertSource = R"(
#version 330 core

out vec2 v_uv;

void main() {
    // Full-screen quad from gl_VertexID (triangle strip: 4 vertices)
    // Vertices: (-1,-1), (1,-1), (-1,1), (1,1)
    vec2 pos = vec2(
        float((gl_VertexID & 1) * 2 - 1),
        float((gl_VertexID >> 1) * 2 - 1)
    );
    v_uv = pos * 0.5 + 0.5; // Map from [-1,1] to [0,1]
    gl_Position = vec4(pos, 0.999, 1.0); // Far depth
}
)";

static const char* s_bgFragSource = R"(
#version 330 core

in vec2 v_uv;

uniform vec3 u_topColor;
uniform vec3 u_bottomColor;

out vec4 fragColor;

void main() {
    vec3 color = mix(u_bottomColor, u_topColor, v_uv.y);
    fragColor = vec4(color, 1.0);
}
)";

BackgroundRenderer::BackgroundRenderer() {}

BackgroundRenderer::~BackgroundRenderer()
{
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

bool BackgroundRenderer::initialize()
{
    // Compile vertex shader
    unsigned int vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &s_bgVertSource, nullptr);
    glCompileShader(vertShader);

    int success = 0;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
        std::fprintf(stderr, "BackgroundRenderer vertex shader compilation failed: %s\n", infoLog);
        glDeleteShader(vertShader);
        return false;
    }

    // Compile fragment shader
    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &s_bgFragSource, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
        std::fprintf(stderr, "BackgroundRenderer fragment shader compilation failed: %s\n", infoLog);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return false;
    }

    // Link program
    m_program = glCreateProgram();
    glAttachShader(m_program, vertShader);
    glAttachShader(m_program, fragShader);
    glLinkProgram(m_program);

    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::fprintf(stderr, "BackgroundRenderer shader linking failed: %s\n", infoLog);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    // Cache uniform locations
    m_locTopColor = glGetUniformLocation(m_program, "u_topColor");
    m_locBottomColor = glGetUniformLocation(m_program, "u_bottomColor");

    // Create an empty VAO (required for gl_VertexID draw calls)
    glGenVertexArrays(1, &m_vao);

    return true;
}

void BackgroundRenderer::render()
{
    if (!m_program) return;

    // Disable depth test and depth write for background
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glUseProgram(m_program);
    glUniform3fv(m_locTopColor, 1, glm::value_ptr(m_topColor));
    glUniform3fv(m_locBottomColor, 1, glm::value_ptr(m_bottomColor));

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);

    // Re-enable depth test and depth write for subsequent rendering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void BackgroundRenderer::setTopColor(glm::vec3 color)
{
    m_topColor = color;
}

void BackgroundRenderer::setBottomColor(glm::vec3 color)
{
    m_bottomColor = color;
}

} // namespace materializr
