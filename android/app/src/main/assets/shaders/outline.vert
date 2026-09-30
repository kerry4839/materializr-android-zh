#version 330 core

// Selection outline vertex shader.
// Renders selected objects slightly scaled up along normals for outline effect.
// Used with stencil technique: first pass writes stencil, second pass draws outline.

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_outlineWidth; // Outline thickness in world units

void main() {
    // Expand vertices along their normals to create the outline shell
    vec3 expandedPos = a_position + normalize(a_normal) * u_outlineWidth;
    gl_Position = u_projection * u_view * u_model * vec4(expandedPos, 1.0);
}
