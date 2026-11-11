#pragma once

// UltraScale2D CPU image-processing core.
// Pixel values are linear RGBA floats in [0, 1]. This module is deliberately
// renderer-agnostic so it can be unit-tested before connecting it to GD's GL
// render targets. It does not claim to implement AMD FSR or Intel XeSS.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ultrascale {

struct Pixel {
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;
};

struct Image {
    int width = 0;
    int height = 0;
    std::vector<Pixel> pixels;

    bool valid() const {
        return width > 0 && height > 0 &&
            pixels.size() == static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    }
    Pixel& at(int x, int y) { return pixels[static_cast<std::size_t>(y) * width + x]; }
    const Pixel& at(int x, int y) const { return pixels[static_cast<std::size_t>(y) * width + x]; }
};

enum class Filter { Nearest, Bilinear, Bicubic, Lanczos3 };

inline float clamp01(float v) { return std::clamp(v, 0.f, 1.f); }
inline int clampIndex(int v, int hi) { return std::clamp(v, 0, hi); }

inline Pixel mix(Pixel a, Pixel b, float t) {
    return {a.r+(b.r-a.r)*t, a.g+(b.g-a.g)*t, a.b+(b.b-a.b)*t, a.a+(b.a-a.a)*t};
}
inline Pixel add(Pixel a, Pixel b, float w) {
    return {a.r+b.r*w, a.g+b.g*w, a.b+b.b*w, a.a+b.a*w};
}
inline Pixel scale(Pixel p, float w) { return {p.r*w,p.g*w,p.b*w,p.a*w}; }

inline float sinc(float x) {
    if (std::abs(x) < 1e-5f) return 1.f;
    constexpr float pi = 3.14159265358979323846f;
    const float px = pi*x;
    return std::sin(px)/px;
}
inline float cubicWeight(float x) {
    // Catmull-Rom cubic kernel (a = -0.5).
    x = std::abs(x);
    if (x < 1.f) return 1.5f*x*x*x - 2.5f*x*x + 1.f;
    if (x < 2.f) return -0.5f*x*x*x + 2.5f*x*x - 4.f*x + 2.f;
    return 0.f;
}

inline Image resize(const Image& src, int outW, int outH, Filter filter) {
    if (!src.valid() || outW <= 0 || outH <= 0) return {};
    Image out{outW, outH, std::vector<Pixel>(static_cast<std::size_t>(outW)*outH)};
    const float sx = static_cast<float>(src.width)/outW;
    const float sy = static_cast<float>(src.height)/outH;

    for (int y=0; y<outH; ++y) for (int x=0; x<outW; ++x) {
        const float fx = (x+0.5f)*sx-0.5f;
        const float fy = (y+0.5f)*sy-0.5f;
        if (filter == Filter::Nearest) {
            out.at(x,y) = src.at(clampIndex(static_cast<int>(std::round(fx)),src.width-1),
                                 clampIndex(static_cast<int>(std::round(fy)),src.height-1));
            continue;
        }
        if (filter == Filter::Bilinear) {
            const int x0=static_cast<int>(std::floor(fx)), y0=static_cast<int>(std::floor(fy));
            const float tx=fx-x0, ty=fy-y0;
            Pixel top=mix(src.at(clampIndex(x0,src.width-1),clampIndex(y0,src.height-1)),
                          src.at(clampIndex(x0+1,src.width-1),clampIndex(y0,src.height-1)),tx);
            Pixel bot=mix(src.at(clampIndex(x0,src.width-1),clampIndex(y0+1,src.height-1)),
                          src.at(clampIndex(x0+1,src.width-1),clampIndex(y0+1,src.height-1)),tx);
            out.at(x,y)=mix(top,bot,ty);
            continue;
        }

        const int radius = filter == Filter::Bicubic ? 2 : 3;
        Pixel sum{0,0,0,0};
        float total=0.f;
        for (int j=-radius+1; j<=radius; ++j) for (int i=-radius+1; i<=radius; ++i) {
            const float dx=fx-(std::floor(fx)+i), dy=fy-(std::floor(fy)+j);
            const float wx = filter == Filter::Bicubic ? cubicWeight(dx) : sinc(dx)*sinc(dx/3.f);
            const float wy = filter == Filter::Bicubic ? cubicWeight(dy) : sinc(dy)*sinc(dy/3.f);
            const float w=wx*wy;
            const auto& p=src.at(clampIndex(static_cast<int>(std::floor(fx))+i,src.width-1),
                                 clampIndex(static_cast<int>(std::floor(fy))+j,src.height-1));
            sum=add(sum,p,w); total+=w;
        }
        if (std::abs(total)>1e-6f) sum=scale(sum,1.f/total);
        out.at(x,y)={clamp01(sum.r),clamp01(sum.g),clamp01(sum.b),clamp01(sum.a)};
    }
    return out;
}

// Contrast-adaptive sharpening inspired by the CAS family of filters.
// This is an original lightweight spatial sharpen pass, NOT AMD's licensed
// FidelityFX CAS implementation and not an upscaler by itself.
inline Image sharpenAdaptive(const Image& src, float strength) {
    if (!src.valid()) return {};
    strength=std::clamp(strength,0.f,1.f);
    Image out=src;
    for (int y=0;y<src.height;++y) for (int x=0;x<src.width;++x) {
        const Pixel c=src.at(x,y);
        const Pixel n=src.at(x,clampIndex(y-1,src.height-1));
        const Pixel s=src.at(x,clampIndex(y+1,src.height-1));
        const Pixel w=src.at(clampIndex(x-1,src.width-1),y);
        const Pixel e=src.at(clampIndex(x+1,src.width-1),y);
        const float lo=std::min({c.r,c.g,c.b,n.r,n.g,n.b,s.r,s.g,s.b,w.r,w.g,w.b,e.r,e.g,e.b});
        const float hi=std::max({c.r,c.g,c.b,n.r,n.g,n.b,s.r,s.g,s.b,w.r,w.g,w.b,e.r,e.g,e.b});
        const float limiter=1.f/(1.f+hi-lo);
        const float k=0.20f*strength*limiter;
        Pixel p{
            c.r+(c.r-(n.r+s.r+w.r+e.r)*0.25f)*k,
            c.g+(c.g-(n.g+s.g+w.g+e.g)*0.25f)*k,
            c.b+(c.b-(n.b+s.b+w.b+e.b)*0.25f)*k,
            c.a
        };
        out.at(x,y)={clamp01(p.r),clamp01(p.g),clamp01(p.b),p.a};
    }
    return out;
}

inline Image process(const Image& input, int width, int height, Filter filter, float sharpness) {
    return sharpenAdaptive(resize(input,width,height,filter),sharpness);
}

} // namespace ultrascale
