#pragma once

#include "FFGLSDK.h"

#include <nozzle/backends/opengl.hpp>
#include <nozzle/result.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace nozzle_ffgl {

constexpr const char *application_name_default = "FFGL";
constexpr const char *receive_sender_default = "nozzle";
constexpr const char *send_sender_default = "nozzle_ffgl";
constexpr uint32_t fallback_width_default = 1280;
constexpr uint32_t fallback_height_default = 720;

std::string non_empty_or_default(const std::string &value, const char *fallback);
uint32_t clamp_dimension(float value, uint32_t fallback);
uint64_t clamp_timeout_ms(float value);
nozzle::texture_origin texture_origin_from_top_left(bool top_left_enabled);

bool ensure_rgba8_texture(GLuint &texture_name, uint32_t width, uint32_t height);
void destroy_texture(GLuint &texture_name);
class scoped_framebuffer_restore {
public:
    explicit scoped_framebuffer_restore(const ProcessOpenGLStruct *process_data);
    ~scoped_framebuffer_restore();

    scoped_framebuffer_restore(const scoped_framebuffer_restore &) = delete;
    scoped_framebuffer_restore &operator=(const scoped_framebuffer_restore &) = delete;

    void restore_previous() const;
    void restore_ffgl_output() const;

private:
    GLint previous_draw_framebuffer_{0};
    GLint previous_read_framebuffer_{0};
    GLuint host_framebuffer_{0};
};

void bind_ffgl_output_framebuffer(const ProcessOpenGLStruct *process_data);
void clear_current_framebuffer_black();

} // namespace nozzle_ffgl
