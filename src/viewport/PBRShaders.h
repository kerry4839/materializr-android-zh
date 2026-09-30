#pragma once

namespace materializr {
namespace shaders {

static const char* pbr_vert = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_worldPos;
out vec3 v_worldNormal;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    v_worldNormal = normalize(normalMatrix * a_normal);
    gl_Position = u_projection * u_view * worldPos;
}
)";

static const char* pbr_frag = R"(
#version 330 core
in vec3 v_worldPos;
in vec3 v_worldNormal;

uniform vec3 u_viewPos;
uniform vec3 u_lightDir;
uniform vec3 u_objectColor;
uniform float u_roughness;
uniform float u_metallic;
uniform bool u_selected;

out vec4 fragColor;

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz normal distribution
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 0.0001);
}

// Schlick-Beckmann geometry function
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(v_worldNormal);
    vec3 V = normalize(u_viewPos - v_worldPos);
    vec3 L = normalize(u_lightDir);
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), u_objectColor, u_metallic);

    // Cook-Torrance BRDF
    float D = distributionGGX(N, H, u_roughness);
    float G = geometrySmith(N, V, L, u_roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - u_metallic);
    float NdotL = max(dot(N, L), 0.0);

    // Simple ambient (IBL would go here)
    vec3 ambient = vec3(0.15) * u_objectColor;

    // Add a fill light from below-left for softer look
    vec3 fillDir = normalize(vec3(-0.5, -0.3, -0.5));
    float fillNdotL = max(dot(N, fillDir), 0.0);
    vec3 fillColor = vec3(0.06) * u_objectColor * fillNdotL;

    vec3 result = ambient + (kD * u_objectColor / PI + specular) * vec3(2.5) * NdotL + fillColor;

    // Tone mapping (Reinhard)
    result = result / (result + vec3(1.0));
    // Gamma correction
    result = pow(result, vec3(1.0/2.2));

    if (u_selected) {
        result = mix(result, vec3(0.3, 0.5, 1.0), 0.3);
    }

    fragColor = vec4(result, 1.0);
}
)";

} // namespace shaders
} // namespace materializr
