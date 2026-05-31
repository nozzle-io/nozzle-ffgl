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

scoped_framebuffer_restore::scoped_framebuffer_restore(const ProcessOpenGLStruct *process_data) {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw_framebuffer_);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read_framebuffer_);
    if (process_data != nullptr) {
        host_framebuffer_ = process_data->HostFBO;
    }
}

scoped_framebuffer_restore::~scoped_framebuffer_restore() {
    restore_previous();
}

void scoped_framebuffer_restore::restore_previous() const {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previous_draw_framebuffer_));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previous_read_framebuffer_));
}

void scoped_framebuffer_restore::restore_ffgl_output() const {
    GLuint draw_framebuffer = host_framebuffer_ != 0
        ? host_framebuffer_
        : static_cast<GLuint>(previous_draw_framebuffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previous_read_framebuffer_));
}

void bind_ffgl_output_framebuffer(const ProcessOpenGLStruct *process_data) {
    if (process_data != nullptr && process_data->HostFBO != 0) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, process_data->HostFBO);
    }
}

void clear_current_framebuffer_black() {
    GLfloat previous_clear_color[4]{};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previous_clear_color);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(
        previous_clear_color[0],
        previous_clear_color[1],
        previous_clear_color[2],
        previous_clear_color[3]
    );
}

} // namespace nozzle_ffgl
