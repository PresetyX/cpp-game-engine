#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "engine/assets/image.hpp"
#include "engine/assets/mesh.hpp"
#include "engine/assets/result.hpp"
#include "engine/assets/shader_source.hpp"
#include "engine/math/glm.hpp"

// OpenGL object wrappers. All of them need a current GL context: create and destroy them on the
// thread that owns the Window, before the Window is destroyed. All are move-only.
namespace engine::render {

// Logs glGetError() with `where` as context (Debug builds only; free in Release).
void check_gl_error(const char* where);

class ShaderProgram {
public:
    // Compiles and links. On failure the error has one "file:line: error: message" per line,
    // `label` being the shader path without extension (".vert"/".frag" are appended).
    static assets::LoadResult<ShaderProgram> create(const assets::ShaderSource& source,
                                                    std::string_view label);

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ~ShaderProgram();

    void use() const;

    // Setters apply to the program bound with use(). Unknown names are ignored (the GLSL
    // compiler drops unused uniforms), which is what hot reload needs.
    void set_int(const std::string& name, int v);
    void set_float(const std::string& name, float v);
    void set_vec3(const std::string& name, const glm::vec3& v);
    void set_vec3_array(const std::string& name, const glm::vec3* v, int count);
    void set_float_array(const std::string& name, const float* v, int count);
    void set_mat3(const std::string& name, const glm::mat3& v);
    void set_mat4(const std::string& name, const glm::mat4& v);

    std::uint32_t id() const { return id_; }

private:
    explicit ShaderProgram(std::uint32_t id) : id_(id) {}
    int location(const std::string& name);

    std::uint32_t id_ = 0;
    std::unordered_map<std::string, int> locations_;
};

class GpuMesh {
public:
    // Layout: attribute 0 = position, 1 = normal, 2 = uv; 32-bit indices.
    explicit GpuMesh(const assets::Mesh& mesh);

    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    ~GpuMesh();

    void draw() const;
    int index_count() const { return index_count_; }

private:
    void release();

    std::uint32_t vao_ = 0;
    std::uint32_t vbo_ = 0;
    std::uint32_t ebo_ = 0;
    int index_count_ = 0;
};

class GpuTexture {
public:
    // RGBA8 with mipmaps, repeat wrapping. Rows are flipped so that uv (0,0) is the bottom-left
    // corner of the picture (see flip_rows). An invalid image gives a 1x1 magenta texture.
    explicit GpuTexture(const assets::Image& image);
    // 1x1 texture of a single color (used when a mesh has no texture).
    static GpuTexture solid(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);

    GpuTexture(GpuTexture&& other) noexcept;
    GpuTexture& operator=(GpuTexture&& other) noexcept;
    GpuTexture(const GpuTexture&) = delete;
    GpuTexture& operator=(const GpuTexture&) = delete;
    ~GpuTexture();

    void bind(unsigned unit) const;

private:
    GpuTexture() = default;
    void upload(int width, int height, const std::uint8_t* rgba);
    void release();

    std::uint32_t id_ = 0;
};

}  // namespace engine::render
