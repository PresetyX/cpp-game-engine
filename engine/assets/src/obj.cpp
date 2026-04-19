#include <charconv>
#include <cstddef>
#include <map>
#include <string>
#include <tuple>

#include "engine/assets/file_io.hpp"
#include "engine/assets/mesh.hpp"

namespace engine::assets {
namespace {

struct Cursor {
    std::string_view rest;

    void skip_space() {
        while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) {
            rest.remove_prefix(1);
        }
    }
    // Next whitespace-separated token; empty at the end of the line.
    std::string_view token() {
        skip_space();
        std::size_t n = 0;
        while (n < rest.size() && rest[n] != ' ' && rest[n] != '\t') {
            ++n;
        }
        const std::string_view t = rest.substr(0, n);
        rest.remove_prefix(n);
        return t;
    }
};

bool parse_float(std::string_view s, float& out) {
    if (!s.empty() && s.front() == '+') {
        s.remove_prefix(1);
    }
    const auto r = std::from_chars(s.data(), s.data() + s.size(), out);
    return r.ec == std::errc() && r.ptr == s.data() + s.size();
}

bool parse_floats(Cursor& c, float* out, int min_count, int max_count, int& got) {
    got = 0;
    while (got < max_count) {
        const std::string_view t = c.token();
        if (t.empty()) {
            break;
        }
        if (!parse_float(t, out[got])) {
            return false;
        }
        ++got;
    }
    return got >= min_count && c.token().empty();
}

// One `f` corner after resolving to 0-based indices; -1 means absent.
struct Corner {
    long v = -1;
    long vt = -1;
    long vn = -1;
    bool operator<(const Corner& o) const {
        return std::tie(v, vt, vn) < std::tie(o.v, o.vt, o.vn);
    }
};

// Parses one index. `count` is the number of elements defined so far (negative indices are
// relative to it). Empty text is "absent" (present == false). False on malformed/out of range.
bool parse_index(std::string_view s, std::size_t count, long& out, bool& present) {
    present = !s.empty();
    if (!present) {
        return true;
    }
    long value = 0;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), value);
    if (r.ec != std::errc() || r.ptr != s.data() + s.size() || value == 0) {
        return false;
    }
    const long n = static_cast<long>(count);
    value = value < 0 ? n + value : value - 1;
    if (value < 0 || value >= n) {
        return false;
    }
    out = value;
    return true;
}

// "v", "v/vt", "v//vn" or "v/vt/vn".
bool parse_corner(std::string_view t, std::size_t nv, std::size_t nvt, std::size_t nvn,
                  Corner& c) {
    std::string_view parts[3];
    int n = 0;
    for (;;) {
        const std::size_t slash = t.find('/');
        parts[n++] = t.substr(0, slash);
        if (slash == std::string_view::npos) {
            break;
        }
        if (n == 3) {
            return false;  // more than two slashes
        }
        t.remove_prefix(slash + 1);
    }
    bool present = false;
    if (!parse_index(parts[0], nv, c.v, present) || !present) {
        return false;
    }
    if (n >= 2 && !parse_index(parts[1], nvt, c.vt, present)) {
        return false;
    }
    if (n >= 3 && !parse_index(parts[2], nvn, c.vn, present)) {
        return false;
    }
    return true;
}

LoadResult<Mesh> fail(std::size_t line, const std::string& what) {
    return LoadResult<Mesh>::failure("obj line " + std::to_string(line) + ": " + what);
}

}  // namespace

LoadResult<Mesh> parse_obj(std::string_view input) {
    const std::string text = normalize_text(std::string(input));  // CRLF -> LF, BOM removed

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    Mesh mesh;
    std::map<Corner, std::uint32_t> lookup;
    std::vector<bool> has_normal;  // per output vertex

    std::size_t line_no = 0;
    std::size_t pos = 0;
    while (pos < text.size()) {
        ++line_no;
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) {
            eol = text.size();
        }
        Cursor c{std::string_view(text).substr(pos, eol - pos)};
        pos = eol + 1;

        const std::string_view cmd = c.token();
        if (cmd.empty() || cmd.front() == '#') {
            continue;
        }
        float f[4];
        int got = 0;
        if (cmd == "v") {
            if (!parse_floats(c, f, 3, 4, got)) {
                return fail(line_no, "bad vertex position");
            }
            positions.emplace_back(f[0], f[1], f[2]);
        } else if (cmd == "vt") {
            if (!parse_floats(c, f, 1, 3, got)) {
                return fail(line_no, "bad texture coordinate");
            }
            uvs.emplace_back(f[0], got >= 2 ? f[1] : 0.0f);
        } else if (cmd == "vn") {
            if (!parse_floats(c, f, 3, 3, got)) {
                return fail(line_no, "bad normal");
            }
            normals.emplace_back(f[0], f[1], f[2]);
        } else if (cmd == "f") {
            std::vector<std::uint32_t> face;
            for (std::string_view t = c.token(); !t.empty(); t = c.token()) {
                Corner corner;
                if (!parse_corner(t, positions.size(), uvs.size(), normals.size(), corner)) {
                    return fail(line_no,
                                "bad or out-of-range face index '" + std::string(t) + "'");
                }
                auto it = lookup.find(corner);
                if (it == lookup.end()) {
                    Vertex v;
                    v.position = positions[static_cast<std::size_t>(corner.v)];
                    if (corner.vt >= 0) {
                        v.uv = uvs[static_cast<std::size_t>(corner.vt)];
                    }
                    if (corner.vn >= 0) {
                        v.normal = normals[static_cast<std::size_t>(corner.vn)];
                    }
                    has_normal.push_back(corner.vn >= 0);
                    it = lookup
                             .emplace(corner, static_cast<std::uint32_t>(mesh.vertices.size()))
                             .first;
                    mesh.vertices.push_back(v);
                }
                face.push_back(it->second);
            }
            if (face.size() < 3) {
                return fail(line_no, "face with fewer than 3 vertices");
            }
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                mesh.indices.push_back(face[0]);
                mesh.indices.push_back(face[i]);
                mesh.indices.push_back(face[i + 1]);
            }
        }
        // Everything else (o, g, s, usemtl, mtllib, ...) is ignored on purpose.
    }

    if (mesh.indices.empty()) {
        return LoadResult<Mesh>::failure("obj: no faces");
    }

    // Smooth normals for vertices that had none: area-weighted sum of adjacent face normals.
    bool any_missing = false;
    for (const bool h : has_normal) {
        any_missing = any_missing || !h;
    }
    if (any_missing) {
        std::vector<glm::vec3> accum(mesh.vertices.size(), glm::vec3(0.0f));
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const std::uint32_t a = mesh.indices[i];
            const std::uint32_t b = mesh.indices[i + 1];
            const std::uint32_t d = mesh.indices[i + 2];
            const glm::vec3 n = glm::cross(mesh.vertices[b].position - mesh.vertices[a].position,
                                           mesh.vertices[d].position - mesh.vertices[a].position);
            accum[a] += n;
            accum[b] += n;
            accum[d] += n;
        }
        for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
            const float len = glm::length(accum[i]);
            if (!has_normal[i] && len > 0.0f) {
                mesh.vertices[i].normal = accum[i] / len;
            }  // degenerate: keep the default normal
        }
    }
    return mesh;
}

LoadResult<Mesh> load_obj(const std::filesystem::path& path) {
    const auto text = read_text_file(path);
    if (!text) {
        return LoadResult<Mesh>::failure(text.error());
    }
    auto mesh = parse_obj(*text);
    if (!mesh) {
        return LoadResult<Mesh>::failure(path.generic_string() + ": " + mesh.error());
    }
    return mesh;
}

}  // namespace engine::assets
