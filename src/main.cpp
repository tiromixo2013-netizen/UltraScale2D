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
}

$on_mod(Loaded) {
    if (runImagePipelineSelfTest()) {
        log::info("UltraScale2D: spatial image pipeline self-test passed (Nearest, Bilinear, Bicubic, Lanczos3, adaptive sharpening).");
    } else {
        log::error("UltraScale2D: image pipeline self-test failed.");
    }
    log::warn("UltraScale2D: the CPU image-processing core is implemented; game-framebuffer/GL integration is not enabled yet. FSR2, XeSS and frame generation are not implemented.");
}
