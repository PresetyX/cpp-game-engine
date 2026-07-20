#include "engine/render/gl_objects.hpp"

#include <glad/gl.h>

#include <cstddef>
#include <utility>
#include <vector>

#include "engine/core/log.hpp"
#include "engine/render/image_upload.hpp"
#include "engine/render/shader_log.hpp"

namespace engine::render {

void check_gl_error(const char* where) {
#ifndef NDEBUG
    for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
        log_error("OpenGL error 0x", std::hex, err, std::dec, " in ", where);
    }
#else
    (void)where;
#endif
}

// ---------------------------------------------------------------- ShaderProgram

namespace {

std::string info_log(GLuint object, bool is_program) {
    GLint length = 0;
    if (is_program) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &length);
    } else {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &length);
    }
    if (length <= 1) {
        return {};
    }
    std::string log(static_cast<std::size_t>(length), '\0');
    GLsizei written = 0;
    if (is_program) {
        glGetProgramInfoLog(object, length, &written, log.data());
    } else {
        glGetShaderInfoLog(object, length, &written, log.data());
    }
    log.resize(static_cast<std::size_t>(written));
    return log;
}

// Returns 0 and fills `error` on failure.
GLuint compile_stage(GLenum type, const std::string& source, const std::string& file,
                     std::string& error) {
    const GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    const std::string log = info_log(shader, false);
    if (ok != GL_TRUE) {
        error = format_diagnostics(parse_info_log(log), file);
        if (error.empty()) {
            error = file + ": compilation failed (the driver gave no message)\n";
        }
        glDeleteShader(shader);
        return 0;
    }
    if (!log.empty()) {
        log_warn(format_diagnostics(parse_info_log(log), file));
    }
    return shader;
}

}  // namespace

assets::LoadResult<ShaderProgram> ShaderProgram::create(const assets::ShaderSource& source,
                                                        std::string_view label) {
    using Result = assets::LoadResult<ShaderProgram>;
    const std::string base(label);
    std::string error;

    const GLuint vs = compile_stage(GL_VERTEX_SHADER, source.vertex, base + ".vert", error);
    if (vs == 0) {
        return Result::failure(error);
    }
    std::string fs_error;
    const GLuint fs = compile_stage(GL_FRAGMENT_SHADER, source.fragment, base + ".frag", fs_error);
    if (fs == 0) {
        glDeleteShader(vs);
        return Result::failure(fs_error);
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        std::string message = format_diagnostics(parse_info_log(info_log(program, true)), base);
        if (message.empty()) {
            message = base + ": link failed (the driver gave no message)\n";
        }
        glDeleteProgram(program);
        return Result::failure(message);
    }
    check_gl_error("ShaderProgram::create");
    return ShaderProgram(program);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : id_(std::exchange(other.id_, 0)), locations_(std::move(other.locations_)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        if (id_ != 0) {
            glDeleteProgram(id_);
        }
        id_ = std::exchange(other.id_, 0);
        locations_ = std::move(other.locations_);
    }
    return *this;
}

ShaderProgram::~ShaderProgram() {
    if (id_ != 0) {
        glDeleteProgram(id_);
    }
}

void ShaderProgram::use() const {
    glUseProgram(id_);
}

int ShaderProgram::location(const std::string& name) {
    const auto it = locations_.find(name);
    if (it != locations_.end()) {
        return it->second;
    }
    const int loc = glGetUniformLocation(id_, name.c_str());
    locations_.emplace(name, loc);
    return loc;
}

void ShaderProgram::set_int(const std::string& name, int v) {
    glUniform1i(location(name), v);
}
void ShaderProgram::set_float(const std::string& name, float v) {
    glUniform1f(location(name), v);
}
void ShaderProgram::set_vec3(const std::string& name, const glm::vec3& v) {
    glUniform3f(location(name), v.x, v.y, v.z);
}
void ShaderProgram::set_vec3_array(const std::string& name, const glm::vec3* v, int count) {
    glUniform3fv(location(name), count, &v[0].x);
}
void ShaderProgram::set_float_array(const std::string& name, const float* v, int count) {
    glUniform1fv(location(name), count, v);
}
void ShaderProgram::set_mat3(const std::string& name, const glm::mat3& v) {
    glUniformMatrix3fv(location(name), 1, GL_FALSE, &v[0][0]);
}
void ShaderProgram::set_mat4(const std::string& name, const glm::mat4& v) {
    glUniformMatrix4fv(location(name), 1, GL_FALSE, &v[0][0]);
}

// ---------------------------------------------------------------- GpuMesh

GpuMesh::GpuMesh(const assets::Mesh& mesh) {
    static_assert(sizeof(assets::Vertex) == 8 * sizeof(float), "Vertex must be tightly packed");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    index_count_ = static_cast<int>(mesh.indices.size());

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(assets::Vertex)),
                 mesh.vertices.data(), GL_STATIC_DRAW);
    // The element buffer binding is part of the VAO state: keep the VAO bound while setting it.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    const auto stride = static_cast<GLsizei>(sizeof(assets::Vertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(offsetof(assets::Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(offsetof(assets::Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(offsetof(assets::Vertex, uv)));
    glBindVertexArray(0);
    check_gl_error("GpuMesh");
}

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : vao_(std::exchange(other.vao_, 0)),
      vbo_(std::exchange(other.vbo_, 0)),
      ebo_(std::exchange(other.ebo_, 0)),
      index_count_(std::exchange(other.index_count_, 0)) {}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept {
    if (this != &other) {
        release();
        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        ebo_ = std::exchange(other.ebo_, 0);
        index_count_ = std::exchange(other.index_count_, 0);
    }
    return *this;
}

GpuMesh::~GpuMesh() {
    release();
}

void GpuMesh::release() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ebo_);
        vao_ = vbo_ = ebo_ = 0;
    }
}

void GpuMesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
}

// ---------------------------------------------------------------- GpuTexture

GpuTexture::GpuTexture(const assets::Image& image) {
    const std::vector<std::uint8_t> flipped = flip_rows(image);
    if (flipped.empty()) {
        log_warn("GpuTexture: invalid image, using a placeholder");
        const std::uint8_t magenta[4] = {255, 0, 255, 255};
        upload(1, 1, magenta);
        return;
    }
    upload(image.width, image.height, flipped.data());
}

GpuTexture GpuTexture::solid(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    GpuTexture t;
    const std::uint8_t px[4] = {r, g, b, a};
    t.upload(1, 1, px);
    return t;
}

void GpuTexture::upload(int width, int height, const std::uint8_t* rgba) {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_gl_error("GpuTexture");
}

GpuTexture::GpuTexture(GpuTexture&& other) noexcept : id_(std::exchange(other.id_, 0)) {}

GpuTexture& GpuTexture::operator=(GpuTexture&& other) noexcept {
    if (this != &other) {
        release();
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

GpuTexture::~GpuTexture() {
    release();
}

void GpuTexture::release() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
}

void GpuTexture::bind(unsigned unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

}  // namespace engine::render
