#include "nozzle_ffgl_common.hpp"

#include <nozzle/receiver.hpp>

#include <memory>
#include <string>
#include <utility>

namespace {

constexpr const char *plugin_id = "NZR1";
constexpr const char *plugin_name = "NozzleReceive";

const char receive_vertex_shader[] = R"(#version 410 core
uniform vec2 maxUV;

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;

out vec2 uv;

void main()
{
    gl_Position = vPosition;
    uv = vUV * maxUV;
}
)";

const char receive_fragment_shader[] = R"(#version 410 core
uniform sampler2D inputTexture;

in vec2 uv;
out vec4 fragColor;

void main()
{
    fragColor = texture(inputTexture, uv);
}
)";

class nozzle_receive_plugin : public ffglqs::Source {
public:
    nozzle_receive_plugin()
    : ffglqs::Source(true)
    {
        sender_name_param_ = ffglqs::ParamText::create("Sender", nozzle_ffgl::receive_sender_default);
        timeout_ms_param_ = ffglqs::ParamRange::CreateInteger("TimeoutMs", 0, ffglqs::ParamRange::Range(0.0f, 1000.0f));
        fallback_width_param_ = ffglqs::ParamRange::CreateInteger("Width", static_cast<int>(nozzle_ffgl::fallback_width_default), ffglqs::ParamRange::Range(1.0f, 16384.0f));
        fallback_height_param_ = ffglqs::ParamRange::CreateInteger("Height", static_cast<int>(nozzle_ffgl::fallback_height_default), ffglqs::ParamRange::Range(1.0f, 16384.0f));

        AddParam(sender_name_param_);
        AddParam(timeout_ms_param_);
        AddParam(fallback_width_param_);
        AddParam(fallback_height_param_);
    }

    FFResult InitGL(const FFGLViewportStruct *view_port) override {
        if (!shader_.Compile(receive_vertex_shader, receive_fragment_shader)) {
            DeInitGL();
            return FF_FAIL;
        }
        if (!quad_.Initialise()) {
            DeInitGL();
            return FF_FAIL;
        }
        return CFFGLPlugin::InitGL(view_port);
    }

    FFResult ProcessOpenGL(ProcessOpenGLStruct *process_data) override {
        (void)process_data;
        ensure_receiver();

        if (!receiver_.valid()) {
            draw_fallback();
            return FF_SUCCESS;
        }

        nozzle::acquire_desc acquire_desc{};
        acquire_desc.timeout_ms = nozzle_ffgl::clamp_timeout_ms(timeout_ms_param_->GetValue());
        auto frame_result = receiver_.acquire_frame(acquire_desc);
        if (!frame_result.ok()) {
            draw_fallback();
            return FF_SUCCESS;
        }

        nozzle::frame frame = std::move(frame_result.value());
        nozzle::frame_info info = frame.info();
        if (info.width == 0 || info.height == 0 || info.format != nozzle::texture_format::rgba8_unorm) {
            frame.release();
            draw_fallback();
            return FF_SUCCESS;
        }

        if (texture_name_ == 0 || texture_width_ != info.width || texture_height_ != info.height) {
            nozzle_ffgl::destroy_texture(texture_name_);
            if (!nozzle_ffgl::ensure_rgba8_texture(texture_name_, info.width, info.height)) {
                frame.release();
                draw_fallback();
                return FF_SUCCESS;
            }
            texture_width_ = info.width;
            texture_height_ = info.height;
        }

        bool top_left_enabled = GetTextureOrientation() == CFFGLPluginManager::TextureOrientation::TOP_LEFT;
        nozzle::gl::gl_texture_desc texture_desc{};
        texture_desc.name = texture_name_;
        texture_desc.target = nozzle::gl::kGLTexture2D;
        texture_desc.width = info.width;
        texture_desc.height = info.height;
        texture_desc.format = nozzle::texture_format::rgba8_unorm;
        texture_desc.origin = nozzle_ffgl::texture_origin_from_top_left(top_left_enabled);

        auto copy_result = nozzle::gl::copy_frame_to_gl_texture(frame, texture_desc);
        frame.release();
        if (!copy_result.ok()) {
            draw_fallback();
            return FF_SUCCESS;
        }

        draw_texture();
        return FF_SUCCESS;
    }

    FFResult DeInitGL() override {
        nozzle_ffgl::destroy_texture(texture_name_);
        texture_width_ = 0;
        texture_height_ = 0;
        shader_.FreeGLResources();
        quad_.Release();
        receiver_ = nozzle::receiver{};
        return FF_SUCCESS;
    }

private:
    void ensure_receiver() {
        std::string requested_name = nozzle_ffgl::non_empty_or_default(sender_name_param_->text, nozzle_ffgl::receive_sender_default);
        if (receiver_.valid() && requested_name == receiver_name_) {
            return;
        }

        receiver_ = nozzle::receiver{};
        receiver_name_ = requested_name;

        nozzle::receiver_desc receiver_desc{};
        receiver_desc.name = receiver_name_;
        receiver_desc.application_name = nozzle_ffgl::application_name_default;
        receiver_desc.receive_mode_val = nozzle::receive_mode::latest_only;

        auto receiver_result = nozzle::receiver::create(receiver_desc);
        if (receiver_result.ok()) {
            receiver_ = std::move(receiver_result.value());
        }
    }

    void draw_texture() {
        ffglex::ScopedShaderBinding shader_binding(shader_.GetGLID());
        ffglex::ScopedSamplerActivation sampler_activation(0);
        ffglex::Scoped2DTextureBinding texture_binding(texture_name_);
        shader_.Set("inputTexture", 0);
        shader_.Set("maxUV", 1.0f, 1.0f);
        quad_.Draw();
    }

    void draw_fallback() {
        uint32_t width = nozzle_ffgl::clamp_dimension(fallback_width_param_->GetValue(), nozzle_ffgl::fallback_width_default);
        uint32_t height = nozzle_ffgl::clamp_dimension(fallback_height_param_->GetValue(), nozzle_ffgl::fallback_height_default);
        (void)width;
        (void)height;
        nozzle_ffgl::clear_current_framebuffer_black();
    }

    std::shared_ptr<ffglqs::ParamText> sender_name_param_{};
    std::shared_ptr<ffglqs::ParamRange> timeout_ms_param_{};
    std::shared_ptr<ffglqs::ParamRange> fallback_width_param_{};
    std::shared_ptr<ffglqs::ParamRange> fallback_height_param_{};
    nozzle::receiver receiver_{};
    std::string receiver_name_{};
    ffglex::FFGLShader shader_{};
    ffglex::FFGLScreenQuad quad_{};
    GLuint texture_name_{0};
    uint32_t texture_width_{0};
    uint32_t texture_height_{0};
};

static CFFGLPluginInfo plugin_info(
    PluginFactory<nozzle_receive_plugin>,
    plugin_id,
    plugin_name,
    2,
    1,
    1,
    0,
    FF_SOURCE,
    "Receive a nozzle source and render it as an FFGL source",
    "nozzle FFGL integration"
);

} // namespace
