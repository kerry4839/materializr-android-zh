#version 330 core

// Infinite grid vertex shader.
// Generates a full-screen quad using gl_VertexID (no VBO needed).
// Unprojects corners to world space using inverse VP matrix.

uniform mat4 u_viewProjection;
uniform mat4 u_invViewProjection;

out vec3 v_nearPoint;
out vec3 v_farPoint;

// Full-screen quad vertices generated from gl_VertexID
vec3 gridPlane[6] = vec3[](
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 unprojectPoint(float x, float y, float z) {
    vec4 unprojected = u_invViewProjection * vec4(x, y, z, 1.0);
    return unprojected.xyz / unprojected.w;
}

void main() {
    vec3 p = gridPlane[gl_VertexID];
    // Unproject near and far plane points to get a ray in world space
    v_nearPoint = unprojectPoint(p.x, p.y, 0.0); // near plane
    v_farPoint  = unprojectPoint(p.x, p.y, 1.0); // far plane
    gl_Position = vec4(p, 1.0);
}
