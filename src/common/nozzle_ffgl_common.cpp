#include "nozzle_ffgl_common.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nozzle_ffgl {

std::string non_empty_or_default(const std::string &value, const char *fallback) {
    if (!value.empty()) {
        return value;
    }
    return fallback ? std::string{fallback} : std::string{};
}

uint32_t clamp_dimension(float value, uint32_t fallback) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    if (value < 1.0f) {
        return fallback;
    }
    constexpr float maximum_dimension = 16384.0f;
    float clamped_value = std::min(value, maximum_dimension);
    return static_cast<uint32_t>(std::lround(clamped_value));
}

uint64_t clamp_timeout_ms(float value) {
    if (!std::isfinite(value) || value < 0.0f) {
        return 0;
    }
    constexpr float maximum_timeout_ms = 1000.0f;
    float clamped_value = std::min(value, maximum_timeout_ms);
    return static_cast<uint64_t>(std::lround(clamped_value));
}

nozzle::texture_origin texture_origin_from_top_left(bool top_left_enabled) {
    return top_left_enabled ? nozzle::texture_origin::top_left : nozzle::texture_origin::bottom_left;
}

bool ensure_rgba8_texture(GLuint &texture_name, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return false;
    }

    if (texture_name == 0) {
        glGenTextures(1, &texture_name);
        if (texture_name == 0) {
            return false;
        }
    }

    GLint previous_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

    glBindTexture(GL_TEXTURE_2D, texture_name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_texture));
    return glGetError() == GL_NO_ERROR;
}

void destroy_texture(GLuint &texture_name) {
    if (texture_name != 0) {
        GLuint texture_to_delete = texture_name;
        glDeleteTextures(1, &texture_to_delete);
        texture_name = 0;
    }
}

void clear_current_framebuffer_black() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace nozzle_ffgl
