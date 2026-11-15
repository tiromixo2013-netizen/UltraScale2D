#if defined(__ANDROID__)
// Include GLES 3.1 declarations before Geode/Cocos defines legacy GL name-remapping macros.
#include "gpu_caps.hpp"
#endif
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include "image_pipeline.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

std::string buildDiagnosticReport() {
    std::ostringstream out;
    out << "UltraScale2D GPU Diagnostic Report\n";
    out << "Mod version: 0.2.2\n";
    out << "Image pipeline self-test: " << (runImagePipelineSelfTest() ? "PASS" : "FAIL") << "\n";
#if defined(__ANDROID__)
    const auto caps = ultrascale::gpu::queryCapabilities();
    out << "Platform: Android\n";
    out << "Current EGL context: " << (caps.contextCurrent ? "yes" : "no") << "\n";
    if (!caps.contextCurrent) {
        out << "GPU query: unavailable because no EGL context is current on this thread. Run this check from the in-game settings.\n";
    } else {
        out << "OpenGL ES version: " << (caps.version ? caps.version : "unavailable") << "\n";
        out << "GPU renderer: " << (caps.renderer ? caps.renderer : "unknown") << "\n";
        out << "GPU vendor: " << (caps.vendor ? caps.vendor : "unknown") << "\n";
        out << "OpenGL ES 3.1 compute baseline: " << (caps.es31 ? "yes" : "no") << "\n";
        if (caps.es31) {
            out << "Max compute work-group invocations: " << caps.maxComputeInvocations << "\n";
            out << "Max shader storage buffer bindings: " << caps.maxStorageBindings << "\n";
            out << "Max shader storage block size (bytes): " << caps.maxStorageBlockBytes << "\n";
        }
    }
#else
    out << "Platform: non-Android (GPU probe is Android-only)\n";
#endif
    out << "GPU culling: not implemented\n";
    out << "Frame generation: not implemented\n";
    out << "Live renderer integration: not implemented\n";
    return out.str();
}

std::string wrapReport(const std::string& report, size_t maxChars) {
    std::istringstream input(report);
    std::ostringstream output;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            output << "\n";
            continue;
        }
        while (line.size() > maxChars) {
            size_t split = line.rfind(' ', maxChars);
            if (split == std::string::npos || split == 0) split = maxChars;
            output << line.substr(0, split) << "\n";
            line.erase(0, split);
            while (!line.empty() && line.front() == ' ') line.erase(line.begin());
        }
        output << line << "\n";
    }
    return output.str();
}

class DiagnosticReportPopup final : public Popup {
protected:
    bool setup(std::string const& report) {
        const auto screen = CCDirector::sharedDirector()->getWinSize();
        const float width = std::min(620.f, std::max(280.f, screen.width * 0.88f));
        const float height = std::min(360.f, std::max(190.f, screen.height * 0.82f));
        if (!Popup::init(width, height, "GJ_square05.png")) return false;

        this->setTitle("GPU Diagnostics", "goldFont.fnt", 0.65f, 18.f);

        const float scrollWidth = width - 30.f;
        const float scrollHeight = height - 66.f;
        auto scroll = ScrollLayer::create({scrollWidth, scrollHeight});
        if (!scroll) return false;
        scroll->setPosition({15.f, 18.f});

        const size_t charsPerLine = static_cast<size_t>(
            std::max(42.f, std::min(100.f, scrollWidth / 5.0f))
        );
        const auto wrapped = wrapReport(report, charsPerLine);
        const auto lineCount = static_cast<float>(
            std::count(wrapped.begin(), wrapped.end(), '\n')
        );
        const float contentHeight = std::max(scrollHeight, lineCount * 13.f + 20.f);

        scroll->m_contentLayer->setContentSize({scrollWidth, contentHeight});
        auto label = CCLabelBMFont::create(wrapped.c_str(), "chatFont.fnt");
        if (!label) return false;
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(0.52f);
        label->setPosition({9.f, contentHeight - 10.f});
        scroll->m_contentLayer->addChild(label);
        scroll->moveToTop();

        m_mainLayer->addChild(scroll);
        return true;
    }

public:
    static DiagnosticReportPopup* create(std::string const& report) {
        auto ret = new DiagnosticReportPopup();
        if (ret->setup(report)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

void runGpuDiagnostics() {
    const auto report = buildDiagnosticReport();
    log::info("UltraScale2D diagnostic report:\n{}", report);
    if (auto popup = DiagnosticReportPopup::create(report)) {
        popup->show();
    } else {
        FLAlertLayer::create("UltraScale2D", "Could not create the scrollable GPU report window.", "OK")->show();
    }
}

void exportGpuReport() {
    const auto report = buildDiagnosticReport();
    const auto path = Mod::get()->getSaveDir() / "gpu-report.txt";
    std::ofstream file(path.string(), std::ios::out | std::ios::trunc);
    if (!file) {
        log::error("UltraScale2D: could not write GPU report to {}", path.string());
        FLAlertLayer::create("UltraScale2D - Export Failed", "Could not write gpu-report.txt. Check the mod save-folder permissions.", "OK")->show();
        return;
    }
    file << report;
    file.close();
    if (!file) {
        log::error("UltraScale2D: failed while writing GPU report to {}", path.string());
        FLAlertLayer::create("UltraScale2D - Export Failed", "The report file could not be completed.", "OK")->show();
        return;
    }
    log::info("UltraScale2D: GPU report exported to {}", path.string());
    FLAlertLayer::create(
        "UltraScale2D - Report Exported",
        fmt::format("The diagnostic report was saved here:\n{}", path.string()),
        "OK"
    )->show();
}
}

$on_mod(Loaded) {
    if (runImagePipelineSelfTest()) {
        log::info("UltraScale2D: spatial image pipeline self-test passed (Nearest, Bilinear, Bicubic, Lanczos3, adaptive sharpening).");
    } else {
        log::error("UltraScale2D: image pipeline self-test failed.");
    }

#if defined(__ANDROID__)
    const auto caps = ultrascale::gpu::queryCapabilities();
    if (!caps.contextCurrent) {
        log::warn("UltraScale2D: no current EGL context during mod load; use Run GPU Diagnostics from the mod settings after the game opens.");
    } else {
        log::info("UltraScale2D: GPU context detected; use the mod settings to export a full report.");
    }
#endif

    // Keep this listener alive for the lifetime of the mod.
    static auto diagnosticButtonListener =
        ButtonSettingPressedEventV3(Mod::get(), "diagnostic-actions").listen(
            [](std::string_view button) {
                if (button == "run-gpu-diagnostics") {
                    runGpuDiagnostics();
                } else if (button == "export-gpu-report") {
                    exportGpuReport();
                }
            }
        );

    log::warn("UltraScale2D: GPU culling is not implemented yet; capability detection alone does not change FPS or move rendering work to the GPU.");
}
