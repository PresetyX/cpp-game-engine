#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "engine/render/lights.hpp"

// The Phong shader lives in the scene sample. These tests need no GL: they read the GLSL text and
// check that it agrees with the C++ side (light limit and uniform names set by the Renderer).

namespace {

std::string read(const std::string& name) {
    std::ifstream in(std::string(ENGINE_SAMPLE_SHADER_DIR) + "/" + name, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace

TEST(PhongShader, FilesExistAndStartWithVersion) {
    for (const char* name : {"phong.vert", "phong.frag"}) {
        const std::string text = read(name);
        ASSERT_FALSE(text.empty()) << name;
        EXPECT_EQ(text.rfind("#version 330 core", 0), 0u) << name;
    }
}

TEST(PhongShader, MaxPointLightsMatchesCpp) {
    const std::string frag = read("phong.frag");
    const std::string define = "#define MAX_POINT_LIGHTS " + std::to_string(engine::render::kMaxPointLights);
    EXPECT_NE(frag.find(define), std::string::npos) << "expected: " << define;
}

TEST(PhongShader, DeclaresEveryUniformTheRendererSets) {
    const std::string all = read("phong.vert") + read("phong.frag");
    for (const char* name :
         {"uModel", "uView", "uProjection", "uNormalMatrix", "uViewPos", "uLightDirection",
          "uLightColor", "uAmbient", "uPointCount", "uPointPosition", "uPointColor",
          "uPointRange", "uDiffuse", "uSpecular", "uShininess", "uTexture"}) {
        EXPECT_NE(all.find(std::string("uniform ")), std::string::npos);
        EXPECT_NE(all.find(name), std::string::npos) << name;
    }
}

TEST(PhongShader, VertexAttributeLocationsMatchGpuMeshLayout) {
    const std::string vert = read("phong.vert");
    EXPECT_NE(vert.find("layout(location = 0) in vec3 aPosition"), std::string::npos);
    EXPECT_NE(vert.find("layout(location = 1) in vec3 aNormal"), std::string::npos);
    EXPECT_NE(vert.find("layout(location = 2) in vec2 aUv"), std::string::npos);
}
