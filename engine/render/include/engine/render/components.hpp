#pragma once

#include "engine/assets/asset_manager.hpp"
#include "engine/math/glm.hpp"

namespace engine::render {

// Blinn-Phong material. Colors are linear RGB (no sRGB handling in this step).
struct Material {
    glm::vec3 diffuse{1.0f};
    glm::vec3 specular{0.5f};
    float shininess = 32.0f;
};

// Draws `mesh` at the entity's Transform. An invalid `texture` handle means "no texture"
// (the diffuse color alone is used). Needs a math::Transform; math::PreviousTransform is
// optional and enables interpolation.
struct MeshRenderer {
    assets::Handle<assets::Mesh> mesh;
    assets::Handle<assets::Image> texture;
    Material material;
};

// One per scene is used (the lowest entity index wins if there are several). It has no
// position: only `direction`, the way the light travels (from the light into the scene).
struct DirectionalLight {
    glm::vec3 direction{-0.4f, -1.0f, -0.3f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    glm::vec3 ambient{0.12f};  // scene-wide ambient term, applied even without a directional light
};

// Position comes from the entity's Transform (needs one). Attenuation reaches zero at `range`.
struct PointLight {
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
};

}  // namespace engine::render
