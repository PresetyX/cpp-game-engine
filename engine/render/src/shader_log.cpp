#include "engine/render/shader_log.hpp"

#include <cctype>

namespace engine::render {

namespace {

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

void skip_spaces(std::string_view& s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
        s.remove_prefix(1);
    }
}

bool eat(std::string_view& s, char c) {
    if (!s.empty() && s.front() == c) {
        s.remove_prefix(1);
        return true;
    }
    return false;
}

bool parse_uint(std::string_view& s, int& out) {
    if (s.empty() || !is_digit(s.front())) {
        return false;
    }
    long long v = 0;
    while (!s.empty() && is_digit(s.front())) {
        v = v * 10 + (s.front() - '0');
        if (v > 100000000) {
            v = 100000000;  // saturate, keep consuming digits
        }
        s.remove_prefix(1);
    }
    out = static_cast<int>(v);
    return true;
}

bool starts_with_nocase(std::string_view s, std::string_view word) {
    if (s.size() < word.size()) {
        return false;
    }
    for (std::size_t i = 0; i < word.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) != word[i]) {
            return false;
        }
    }
    return true;
}

bool eat_severity(std::string_view& s, DiagnosticSeverity& sev) {
    if (starts_with_nocase(s, "error")) {
        sev = DiagnosticSeverity::Error;
        s.remove_prefix(5);
        return true;
    }
    if (starts_with_nocase(s, "warning")) {
        sev = DiagnosticSeverity::Warning;
        s.remove_prefix(7);
        return true;
    }
    return false;
}

// After the position part: optional spaces, then the message with a leading ':' removed.
std::string rest_message(std::string_view s) {
    skip_spaces(s);
    eat(s, ':');
    skip_spaces(s);
    return std::string(s);
}

// NVIDIA: 0(12) : error C1008: text
bool parse_nvidia(std::string_view s, ShaderDiagnostic& d) {
    if (!parse_uint(s, d.source) || !eat(s, '(') || !parse_uint(s, d.line) || !eat(s, ')')) {
        return false;
    }
    skip_spaces(s);
    if (!eat(s, ':')) {
        return false;
    }
    skip_spaces(s);
    if (!eat_severity(s, d.severity)) {
        return false;
    }
    d.message = rest_message(s);
    return true;
}

// Mesa: 0:12(5): error: text
bool parse_mesa(std::string_view s, ShaderDiagnostic& d) {
    int column = 0;
    if (!parse_uint(s, d.source) || !eat(s, ':') || !parse_uint(s, d.line)) {
        return false;
    }
    if (eat(s, '(')) {
        if (!parse_uint(s, column) || !eat(s, ')')) {
            return false;
        }
    }
    if (!eat(s, ':')) {
        return false;
    }
    skip_spaces(s);
    if (!eat_severity(s, d.severity)) {
        return false;
    }
    d.message = rest_message(s);
    return true;
}

// Intel/AMD: ERROR: 0:12: text
bool parse_prefixed(std::string_view s, ShaderDiagnostic& d) {
    if (!eat_severity(s, d.severity) || !eat(s, ':')) {
        return false;
    }
    skip_spaces(s);
    if (!parse_uint(s, d.source) || !eat(s, ':') || !parse_uint(s, d.line)) {
        return false;
    }
    d.message = rest_message(s);
    return true;
}

const char* severity_name(DiagnosticSeverity s) {
    switch (s) {
        case DiagnosticSeverity::Error:
            return "error";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Note:
            break;
    }
    return "";
}

}  // namespace

std::vector<ShaderDiagnostic> parse_info_log(std::string_view log) {
    std::vector<ShaderDiagnostic> out;
    while (!log.empty()) {
        const std::size_t nl = log.find('\n');
        std::string_view line = log.substr(0, nl);
        log = nl == std::string_view::npos ? std::string_view{} : log.substr(nl + 1);

        while (!line.empty() && (line.back() == '\r' || line.back() == '\0' || line.back() == ' ')) {
            line.remove_suffix(1);
        }
        skip_spaces(line);
        if (line.empty()) {
            continue;
        }

        ShaderDiagnostic d;
        if (parse_nvidia(line, d) || parse_mesa(line, d) || parse_prefixed(line, d)) {
            out.push_back(std::move(d));
            continue;
        }
        d = ShaderDiagnostic{};
        d.message = std::string(line);
        out.push_back(std::move(d));
    }
    return out;
}

std::string format_diagnostics(const std::vector<ShaderDiagnostic>& diagnostics,
                               std::string_view label) {
    std::string out;
    for (const ShaderDiagnostic& d : diagnostics) {
        out += label;
        if (d.line > 0) {
            out += ':';
            out += std::to_string(d.line);
        }
        out += ": ";
        const std::string_view sev = severity_name(d.severity);
        if (!sev.empty()) {
            out += sev;
            out += ": ";
        }
        out += d.message;
        out += '\n';
    }
    return out;
}

}  // namespace engine::render
