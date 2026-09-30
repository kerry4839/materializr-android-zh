#version 330 core

// Blinn-Phong mesh fragment shader.
// Configurable lighting: ambient strength, optional camera headlight, and an
// optional fill light to lift shadowed faces. Selection highlight tints blue.
// NOTE: the compiled copy is embedded in ShapeRenderer.cpp; keep both in sync.

in vec3 v_worldPos;
in vec3 v_worldNormal;

uniform vec3 u_viewPos;
uniform vec3 u_lightDir;       // key light, direction TO the light (normalized)
uniform vec3 u_fillDir;        // fill light, direction TO the light (normalized)
uniform vec3 u_objectColor;
uniform bool u_selected;
uniform float u_ambient;       // base illumination 0..1 (softens shadows)
uniform bool u_headlight;      // key light tracks the camera when true
uniform float u_fillStrength;  // fill light contribution (0 disables it)
uniform bool u_previewCut;     // tint red: this volume will be subtracted

out vec4 fragColor;

void main() {
    vec3 normal = normalize(v_worldNormal);
    vec3 viewDir = normalize(u_viewPos - v_worldPos);
    // Headlight: the key light comes from the camera, so the face the user is
    // looking at is always lit and large cast shadows disappear.
    vec3 keyDir = u_headlight ? viewDir : normalize(u_lightDir);

    // Ambient
    vec3 ambient = u_ambient * u_objectColor;

    // Diffuse (key + optional fill)
    float keyDiff = max(dot(normal, keyDir), 0.0);
    float fillDiff = max(dot(normal, normalize(u_fillDir)), 0.0) * u_fillStrength;
    vec3 diffuse = (keyDiff + fillDiff) * u_objectColor;

    // Specular (Blinn-Phong)
    float specularStrength = 0.5;
    float shininess = 32.0;
    vec3 halfwayDir = normalize(keyDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;

    // Subtract preview: tint red (this volume will be removed)
    if (u_previewCut) {
        result = mix(result, vec3(0.9, 0.1, 0.1), 0.55);
    }

    // Selection highlight: tint with blue
    if (u_selected) {
        result = mix(result, vec3(0.3, 0.5, 1.0), 0.3);
    }

    fragColor = vec4(result, 1.0);
}
