#version 330 core

// Blinn-Phong: one directional light plus up to MAX_POINT_LIGHTS point lights.
// Keep MAX_POINT_LIGHTS equal to engine::render::kMaxPointLights (a test checks it).
#define MAX_POINT_LIGHTS 8

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vUv;

uniform vec3 uViewPos;
uniform vec3 uLightDirection;  // direction the light travels
uniform vec3 uLightColor;      // color * intensity, black if there is no directional light
uniform vec3 uAmbient;

uniform int uPointCount;
uniform vec3 uPointPosition[MAX_POINT_LIGHTS];
uniform vec3 uPointColor[MAX_POINT_LIGHTS];
uniform float uPointRange[MAX_POINT_LIGHTS];

uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform float uShininess;
uniform sampler2D uTexture;

out vec4 FragColor;

// Diffuse and specular factors of one light. L points from the surface towards the light.
void shade(vec3 N, vec3 V, vec3 L, vec3 color, float attenuation, inout vec3 diffuse,
           inout vec3 specular) {
    float n_dot_l = max(dot(N, L), 0.0);
    diffuse += color * n_dot_l * attenuation;
    if (n_dot_l > 0.0) {
        vec3 H = normalize(L + V);
        specular += color * pow(max(dot(N, H), 0.0), uShininess) * attenuation;
    }
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorldPosition);

    vec3 diffuse = uAmbient;
    vec3 specular = vec3(0.0);

    shade(N, V, normalize(-uLightDirection), uLightColor, 1.0, diffuse, specular);

    for (int i = 0; i < uPointCount; ++i) {
        vec3 to_light = uPointPosition[i] - vWorldPosition;
        float dist = length(to_light);
        float falloff = clamp(1.0 - (dist * dist) / (uPointRange[i] * uPointRange[i]), 0.0, 1.0);
        shade(N, V, to_light / max(dist, 1e-4), uPointColor[i], falloff * falloff, diffuse,
              specular);
    }

    vec4 tex = texture(uTexture, vUv);
    vec3 base = uDiffuse * tex.rgb;
    FragColor = vec4(base * diffuse + uSpecular * specular, 1.0);
}
