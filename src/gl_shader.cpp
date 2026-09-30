// GLSL 330-core -> GLSL ES 3.00 shader-source adapter (Android only).
// On desktop this file compiles to nothing.
#include "gl_common.h"

#if defined(MZ_GLES)

// gl_common.h #defined glShaderSource to our adapter; undo that here so the
// adapter can call the genuine GLES entry point.
#ifdef glShaderSource
#undef glShaderSource
#endif

#include <string>
#include <vector>

namespace materializr {

namespace {

// Rewrite the "#version 330 core" header to "#version 300 es" and inject default
// precision qualifiers. The shader bodies Materializr uses (vec/mat math,
// transpose/inverse, in/out stages, an explicit `out vec4` fragment color) are
// already valid GLSL ES 3.00, so only the header changes.
std::string adaptForGLES(const std::string& in) {
    std::string src = in;

    // GLSL ES requires #version on the very first line. The embedded shaders are
    // raw string literals that begin with a newline (R"(\n#version 330 core...),
    // which desktop GL tolerates but GLSL ES rejects (P0005). Strip any leading
    // whitespace so the directive ends up on line 1.
    {
        std::size_t v = src.find("#version");
        if (v != std::string::npos && v > 0) {
            bool onlyWs = true;
            for (std::size_t i = 0; i < v; ++i) {
                char c = src[i];
                if (c != ' ' && c != '\t' && c != '\r' && c != '\n') { onlyWs = false; break; }
            }
            if (onlyWs) src.erase(0, v);
        }
    }

    auto swap = [&](const char* from, const char* to) -> bool {
        std::size_t p = src.find(from);
        if (p == std::string::npos) return false;
        src.replace(p, std::char_traits<char>::length(from), to);
        return true;
    };

    if (!swap("#version 330 core", "#version 300 es"))
        swap("#version 330", "#version 300 es");

    std::size_t p = src.find("#version 300 es");
    if (p != std::string::npos) {
        std::size_t eol = src.find('\n', p);
        eol = (eol == std::string::npos) ? src.size() : eol + 1;
        src.insert(eol, "precision highp float;\nprecision highp int;\n");
    }
    return src;
}

} // namespace

void glShaderSourceAdapt(GLuint shader, GLsizei count,
                         const GLchar* const* string, const GLint* length) {
    // Reconstruct each source string (honouring explicit lengths when given),
    // adapt it, then forward the adapted copies to the real glShaderSource.
    std::vector<std::string> owned;
    owned.reserve(count > 0 ? count : 0);
    for (GLsizei i = 0; i < count; ++i) {
        if (length && length[i] >= 0)
            owned.emplace_back(string[i], static_cast<std::size_t>(length[i]));
        else
            owned.emplace_back(string[i] ? string[i] : "");
        owned.back() = adaptForGLES(owned.back());
    }

    std::vector<const GLchar*> ptrs(owned.size());
    for (std::size_t i = 0; i < owned.size(); ++i) ptrs[i] = owned[i].c_str();

    glShaderSource(shader, count, ptrs.data(), nullptr);
}

} // namespace materializr

#endif // MZ_GLES
