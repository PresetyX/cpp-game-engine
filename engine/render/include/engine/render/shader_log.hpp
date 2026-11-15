#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace engine::render {

enum class DiagnosticSeverity : std::uint8_t { Error, Warning, Note };

// One message of a GLSL compiler info log. `line` is 1-based in the shader source that was
// passed to glShaderSource; 0 means the driver gave no line. Because the engine does not inject
// text after `#version`, that is also the line in the shader file.
struct ShaderDiagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Note;
    int source = 0;  // GLSL "source string" number (always 0 for a single string)
    int line = 0;
    std::string message;
};

// Parses an info log. Recognized dialects:
//   NVIDIA    0(12) : error C1008: undefined variable "x"
//   Mesa      0:12(5): error: 'x' undeclared
//   Intel/AMD ERROR: 0:12: 'x' : undeclared identifier
// Any other non-empty line is kept as a Note without a line number, so no driver text is lost.
// Never throws.
std::vector<ShaderDiagnostic> parse_info_log(std::string_view log);

// "path:12: error: message", one diagnostic per line, `label` being the shader file path.
// Diagnostics without a line print as "path: error: message" (notes print "path: message").
std::string format_diagnostics(const std::vector<ShaderDiagnostic>& diagnostics,
                               std::string_view label);

}  // namespace engine::render
