#include <ffgl/FFGL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLIOSurface.h>
#include <nozzle/backends/metal.hpp>
#include <nozzle/backends/opengl.hpp>
#include <nozzle/receiver.hpp>
#include <nozzle/pixel_access.hpp>

#include <array>
#include <cstdio>
#include <dlfcn.h>
#include <string>
#include <unistd.h>

namespace {

using pixels = std::array<unsigned char, 8>;
constexpr pixels expected{219, 53, 17, 255, 11, 105, 193, 255};

int fail(const char* reason) {
    std::fprintf(stderr, "%s\n", reason);
    return 1;
}

GLuint texture(const void* data) {
    GLuint name{};
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return name;
}

FFMixed pointer(void* value) {
    FFMixed argument{};
    argument.PointerValue = value;
    return argument;
}

void report_texture(const char* stage, GLenum target, GLuint texture_name) {
    pixels rgba{};
    glBindTexture(target, texture_name);
    glGetTexImage(target, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    const auto error = glGetError();
    if (error != GL_NO_ERROR) {
        std::fprintf(stderr, "%s readback error 0x%x\n", stage, error);
        return;
    }
    std::printf("%s read as RGBA: %u %u %u %u / %u %u %u %u\n", stage,
        rgba[0], rgba[1], rgba[2], rgba[3], rgba[4], rgba[5], rgba[6], rgba[7]);
}

int run(FF_Main_FuncPtr plugin) {
    nozzle::sender_desc desc{};
    desc.name = "ffgl-receive-" + std::to_string(getpid());
    auto sender = nozzle::sender::create(desc);
    if (!sender.ok()) return fail(sender.error().message.c_str());

    nozzle::gl::gl_texture_desc source{};
    source.name = texture(expected.data());
    source.width = 2;
    source.height = 1;
    auto publication = nozzle::gl::publish_gl_texture(sender.value(), source);
    if (!publication.ok()) return fail(publication.error().message.c_str());
    glFinish();

    // Observe the published metadata independently before exercising the real bundle.
    nozzle::receiver_desc receiver_desc{};
    receiver_desc.name = desc.name;
    auto observer = nozzle::receiver::create(receiver_desc);
    if (!observer.ok()) return fail(observer.error().message.c_str());
    auto frame = observer.value().acquire_frame();
    if (!frame.ok()) return fail(frame.error().message.c_str());
    const auto info = frame.value().info();
    if (info.format != nozzle::texture_format::bgra8_unorm ||
        info.semantic_format != nozzle::texture_format::rgba8_unorm ||
        info.width != 2 || info.height != 1) return fail("Unexpected published frame metadata");
    std::puts("Core GL frame acquired: 2x1, semantic RGBA8, storage BGRA8");

    FFGLViewportStruct viewport{0, 0, 2, 1};
    glViewport(0, 0, 2, 1);
    if (plugin(FF_INITIALISE_V2, {}, nullptr).UIntValue != FF_SUCCESS) return fail("FFGL initialize failed");
    const auto instance = plugin(FF_INSTANTIATE_GL, pointer(&viewport), nullptr).PointerValue;
    if (!instance || instance == reinterpret_cast<void*>(FF_FAIL)) return fail("FFGL instantiate failed");
    SetParameterStruct sender_name{0, pointer(desc.name.data())};
    if (plugin(FF_SET_PARAMETER, pointer(&sender_name), instance).UIntValue != FF_SUCCESS)
        return fail("FFGL sender selection failed");

    const auto output = texture(nullptr);
    GLuint fbo{};
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return fail("Incomplete output FBO");
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ProcessOpenGLStruct process{0, nullptr, fbo};
    if (plugin(FF_PROCESS_OPENGL, pointer(&process), instance).UIntValue != FF_SUCCESS)
        return fail("FFGL render failed");

    pixels actual{};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, actual.data());
    if (glGetError() != GL_NO_ERROR) return fail("GL error during render or readback");
    for (size_t i = 0; i < actual.size(); i += 4) {
        std::printf("pixel %zu expected (%u,%u,%u,%u) rendered (%u,%u,%u,%u)\n", i / 4,
            expected[i], expected[i+1], expected[i+2], expected[i+3],
            actual[i], actual[i+1], actual[i+2], actual[i+3]);
    }
    if (actual != expected) {
        // Diagnose the single published frame only after the host readback.
        auto mapping = nozzle::lock_frame_pixels_mapping_with_origin(frame.value(), nozzle::texture_origin::top_left);
        if (mapping.ok()) {
            const auto& mapped = mapping.value().pixels();
            if (mapped.data && mapped.width == 2 && mapped.height == 1 &&
                mapped.format == nozzle::texture_format::bgra8_unorm) {
                const auto* bytes = static_cast<const unsigned char*>(mapped.data);
                std::printf("published BGRA bytes: %u %u %u %u / %u %u %u %u\n",
                    bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
            } else {
                std::fputs("Unexpected source mapping layout\n", stderr);
            }
            mapping.value().unlock();
        } else {
            std::fprintf(stderr, "Cannot inspect source bytes: %s\n", mapping.error().message.c_str());
        }
        auto diagnostic = source;
        diagnostic.name = texture(nullptr);
        diagnostic.format = info.format;
        auto copy = nozzle::gl::copy_frame_to_gl_texture(frame.value(), diagnostic);
        if (!copy.ok()) {
            std::fprintf(stderr, "Core BGRA8 copy: %s\n", copy.error().message.c_str());
        } else {
            report_texture("Core BGRA8 copy", GL_TEXTURE_2D, diagnostic.name);
        }
        glDeleteTextures(1, &diagnostic.name);

        auto surface = static_cast<IOSurfaceRef>(nozzle::metal::get_io_surface(frame.value().get_texture()));
        if (surface) {
            GLuint rectangle{};
            glGenTextures(1, &rectangle);
            glBindTexture(GL_TEXTURE_RECTANGLE, rectangle);
            auto binding = CGLTexImageIOSurface2D(CGLGetCurrentContext(), GL_TEXTURE_RECTANGLE,
                GL_RGBA8, 2, 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, surface, 0);
            std::printf("Packed BGRA IOSurface binding: %s\n", CGLErrorString(binding));
            if (binding == kCGLNoError) {
                report_texture("Packed binding", GL_TEXTURE_RECTANGLE, rectangle);
            }
            glDeleteTextures(1, &rectangle);
        } else {
            std::fputs("No source IOSurface\n", stderr);
        }
    }
    frame.value().release();
    plugin(FF_DEINSTANTIATE_GL, {}, instance);
    plugin(FF_DEINITIALISE, {}, nullptr);
    return actual == expected ? 0 : fail("FFGL output differs from published pixels");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) return fail("Expected NozzleReceive bundle executable path");
    const CGLPixelFormatAttribute attributes[]{kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_GL4_Core), static_cast<CGLPixelFormatAttribute>(0)};
    CGLPixelFormatObj format{};
    GLint count{};
    auto error = CGLChoosePixelFormat(attributes, &format, &count);
    if (error != kCGLNoError || !format) return fail("Cannot select OpenGL 4.1 pixel format");
    CGLContextObj context{};
    error = CGLCreateContext(format, nullptr, &context);
    CGLReleasePixelFormat(format);
    if (error != kCGLNoError || CGLSetCurrentContext(context) != kCGLNoError) return fail("Cannot activate CGL context");
    std::printf("GL renderer: %s\n", glGetString(GL_RENDERER));

    auto* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) return fail(dlerror());
    auto plugin = reinterpret_cast<FF_Main_FuncPtr>(dlsym(library, "plugMain"));
    if (!plugin) return fail(dlerror());
    const int result = run(plugin);
    CGLSetCurrentContext(nullptr);
    CGLReleaseContext(context);
    return result;
}
