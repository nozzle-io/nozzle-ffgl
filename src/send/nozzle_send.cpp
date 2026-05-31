#include "nozzle_ffgl_common.hpp"

#include <nozzle/sender.hpp>

#include <memory>
#include <string>
#include <utility>

namespace {

constexpr const char *plugin_id = "NZS1";
constexpr const char *plugin_name = "NozzleSend";

class nozzle_send_plugin : public ffglqs::Effect {
public:
    nozzle_send_plugin()
    : ffglqs::Effect(true)
    {
        sender_name_param_ = ffglqs::ParamText::create("Sender", nozzle_ffgl::send_sender_default);
        application_name_param_ = ffglqs::ParamText::create("AppName", nozzle_ffgl::application_name_default);
        publish_enabled_param_ = ffglqs::ParamBool::Create("Publish", true);

        AddParam(sender_name_param_);
        AddParam(application_name_param_);
        AddParam(publish_enabled_param_);
        SetFragmentShader(R"(
            void main()
            {
                fragColor = texture(inputTexture, i_uv);
            }
        )");
    }

    FFResult ProcessOpenGL(ProcessOpenGLStruct *process_data) override {
        if (process_data == nullptr || process_data->numInputTextures < 1 || process_data->inputTextures[0] == nullptr) {
            return FF_FAIL;
        }

        if (publish_enabled_param_->GetValue() >= 0.5f) {
            {
                nozzle_ffgl::scoped_framebuffer_restore framebuffer_restore(process_data);
                publish_input_texture(*process_data->inputTextures[0]);
                framebuffer_restore.restore_ffgl_output();
            }
        }

        nozzle_ffgl::bind_ffgl_output_framebuffer(process_data);
        return ffglqs::Effect::ProcessOpenGL(process_data);
    }

    FFResult DeInitGL() override {
        sender_ = nozzle::sender{};
        return ffglqs::Effect::DeInitGL();
    }

private:
    bool ensure_sender() {
        std::string requested_name = nozzle_ffgl::non_empty_or_default(sender_name_param_->text, nozzle_ffgl::send_sender_default);
        std::string requested_application = nozzle_ffgl::non_empty_or_default(application_name_param_->text, nozzle_ffgl::application_name_default);
        if (sender_.valid() && requested_name == sender_name_ && requested_application == application_name_) {
            return true;
        }

        sender_ = nozzle::sender{};
        sender_name_ = requested_name;
        application_name_ = requested_application;

        nozzle::sender_desc sender_desc{};
        sender_desc.name = sender_name_;
        sender_desc.application_name = application_name_;
        sender_desc.ring_buffer_size = 3;
        sender_desc.fallback_flags = nozzle::fallback_safe_defaults;

        auto sender_result = nozzle::sender::create(sender_desc);
        if (sender_result.ok()) {
            sender_ = std::move(sender_result.value());
            record_publish_status("sender ready");
            return true;
        }

        record_publish_status(std::string{"sender creation failed: "} + sender_result.error().message);
        return false;
    }

    void publish_input_texture(const FFGLTextureStruct &input_texture) {
        if (!ensure_sender()) {
            return;
        }
        if (!sender_.valid()) {
            record_publish_status("sender is invalid");
            return;
        }
        if (input_texture.Handle == 0 || input_texture.Width == 0 || input_texture.Height == 0) {
            record_publish_status("input texture is invalid");
            return;
        }

        bool top_left_enabled = GetTextureOrientation() == CFFGLPluginManager::TextureOrientation::TOP_LEFT;
        nozzle::gl::gl_texture_desc texture_desc{};
        texture_desc.name = input_texture.Handle;
        texture_desc.target = nozzle::gl::kGLTexture2D;
        texture_desc.width = input_texture.Width;
        texture_desc.height = input_texture.Height;
        texture_desc.format = nozzle::texture_format::rgba8_unorm;
        texture_desc.origin = nozzle_ffgl::texture_origin_from_top_left(top_left_enabled);

        auto publish_result = nozzle::gl::publish_gl_texture(sender_, texture_desc);
        if (publish_result.ok()) {
            record_publish_status("publish ok");
        } else {
            record_publish_status(std::string{"publish failed: "} + publish_result.error().message);
        }
    }

    void record_publish_status(const std::string &status) {
        if (status == last_publish_status_) {
            return;
        }
        last_publish_status_ = status;
        std::string message = std::string{"NozzleSend: "} + status;
        FFGLLog::LogToHost(message.c_str());
    }

    std::shared_ptr<ffglqs::ParamText> sender_name_param_{};
    std::shared_ptr<ffglqs::ParamText> application_name_param_{};
    std::shared_ptr<ffglqs::ParamBool> publish_enabled_param_{};
    nozzle::sender sender_{};
    std::string sender_name_{};
    std::string application_name_{};
    std::string last_publish_status_{};
};

static CFFGLPluginInfo plugin_info(
    PluginFactory<nozzle_send_plugin>,
    plugin_id,
    plugin_name,
    2,
    1,
    1,
    0,
    FF_EFFECT,
    "Publish an FFGL input texture as a nozzle source and pass it through",
    "nozzle FFGL integration"
);

} // namespace
