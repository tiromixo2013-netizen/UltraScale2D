#pragma once

// Runtime capability probe for the Android OpenGL ES context.
// This file only detects support; it does not dispatch work or alter rendering.
#if defined(__ANDROID__)
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cstdio>

namespace ultrascale::gpu {

struct Capabilities {
    bool contextCurrent = false;
    bool es31 = false;
    int major = 0;
    int minor = 0;
    GLint maxComputeInvocations = 0;
    GLint maxStorageBindings = 0;
    GLint64 maxStorageBlockBytes = 0;
    const char* version = nullptr;
    const char* renderer = nullptr;
    const char* vendor = nullptr;
};

inline Capabilities queryCapabilities() {
    Capabilities caps{};

    // Never issue GL queries unless this thread owns a current EGL context.
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) return caps;
    caps.contextCurrent = true;

    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!version) return caps;
    caps.version = version;
    caps.renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    caps.vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

    if (std::sscanf(version, "OpenGL ES %d.%d", &caps.major, &caps.minor) != 2) {
        caps.major = 0;
        caps.minor = 0;
        return caps;
    }

    caps.es31 = caps.major > 3 || (caps.major == 3 && caps.minor >= 1);
    if (caps.es31) {
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &caps.maxComputeInvocations);
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &caps.maxStorageBindings);
        glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &caps.maxStorageBlockBytes);
    }
    return caps;
}

} // namespace ultrascale::gpu
#endif
