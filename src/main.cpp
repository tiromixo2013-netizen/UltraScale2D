#if defined(__ANDROID__)
// Include GLES 3.1 declarations before Geode/Cocos defines legacy GL name-remapping macros.
#include "gpu_caps.hpp"
#endif
#include <Geode/Geode.hpp>
#include "image_pipeline.hpp"

using namespace geode::prelude;

namespace {
bool runImagePipelineSelfTest() {
    using namespace ultrascale;
    Image test{
        2, 2,
        {
            Pixel{1.f, 0.f, 0.f, 1.f},
            Pixel{0.f, 1.f, 0.f, 1.f},
            Pixel{0.f, 0.f, 1.f, 1.f},
            Pixel{1.f, 1.f, 1.f, 1.f}
        }
    };
    for (const auto filter : {Filter::Nearest, Filter::Bilinear, Filter::Bicubic, Filter::Lanczos3}) {
        const auto result = process(test, 4, 4, filter, 0.35f);
        if (!result.valid() || result.width != 4 || result.height != 4) return false;
    }
    return true;
}

#if defined(__ANDROID__)
void logGpuCapabilities() {
    const auto caps = ultrascale::gpu::queryCapabilities();
    if (!caps.contextCurrent) {
        log::warn("UltraScale2D: no current EGL context during mod load; OpenGL ES capability probe deferred.");
        return;
    }
    if (!caps.version) {
        log::warn("UltraScale2D: EGL context exists but GL_VERSION is unavailable.");
        return;
    }

    log::info("UltraScale2D: GL version: {}", caps.version);
    log::info("UltraScale2D: GPU renderer: {}", caps.renderer ? caps.renderer : "unknown");
    log::info("UltraScale2D: GPU vendor: {}", caps.vendor ? caps.vendor : "unknown");
    if (!caps.es31) {
        log::warn("UltraScale2D: OpenGL ES 3.1 compute/SSBO baseline unavailable; GPU culling must stay disabled.");
        return;
    }
    log::info(
        "UltraScale2D: ES 3.1 compute baseline detected (max invocations: {}, SSBO bindings: {}, max SSBO block: {} bytes).",
        caps.maxComputeInvocations, caps.maxStorageBindings,
        static_cast<long long>(caps.maxStorageBlockBytes)
    );
}
#endif
}

$on_mod(Loaded) {
    if (runImagePipelineSelfTest()) {
        log::info("UltraScale2D: spatial image pipeline self-test passed (Nearest, Bilinear, Bicubic, Lanczos3, adaptive sharpening).");
    } else {
        log::error("UltraScale2D: image pipeline self-test failed.");
    }
#if defined(__ANDROID__)
    logGpuCapabilities();
#endif
    log::warn("UltraScale2D: GPU culling is not implemented yet; capability detection alone does not change FPS or move rendering work to the GPU.");
}
