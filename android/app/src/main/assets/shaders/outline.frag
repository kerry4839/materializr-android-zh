#version 330 core

// Selection outline fragment shader.
// Outputs a solid outline color where stencil test passes (stencil != 1).
// This creates a visible outline around selected geometry.

uniform vec4 u_outlineColor;

out vec4 fragColor;

void main() {
    fragColor = u_outlineColor;
}
