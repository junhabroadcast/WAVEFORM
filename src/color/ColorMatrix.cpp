#include "color/ColorMatrix.h"

namespace ColorMatrix {
namespace {

struct Coeffs {
    float kr, kg, kb;
};

Coeffs coeffsFor(Colorimetry c)
{
    if (c == Colorimetry::BT601)
        return {0.299f, 0.587f, 0.114f};
    return {0.2126f, 0.7152f, 0.0722f}; // BT.709
}

struct Ycc {
    float y, cb, cr;
};

// R'G'B' (0..1) -> Y' 0..1, Cb/Cr -0.5..0.5 — same normalization as normY/normC.
Ycc rgbToYcc(const Coeffs& k, float r, float g, float b)
{
    const float y = k.kr * r + k.kg * g + k.kb * b;
    return {y, (b - y) / (2.0f * (1.0f - k.kb)), (r - y) / (2.0f * (1.0f - k.kr))};
}

struct BarColor {
    const char* name;
    float r, g, b;
};

constexpr BarColor kBarColors[] = {
    {"R", 1, 0, 0},
    {"Mg", 1, 0, 1},
    {"B", 0, 0, 1},
    {"Cy", 0, 1, 1},
    {"G", 0, 1, 0},
    {"Yl", 1, 1, 0},
};

} // namespace

Colorimetry resolve(Colorimetry selected, int height)
{
    if (selected != Colorimetry::Auto)
        return selected;
    return height >= 720 ? Colorimetry::BT709 : Colorimetry::BT601;
}

QVector<VectorTarget> vectorTargets(bool bars75, Colorimetry c)
{
    const Coeffs k = coeffsFor(c);
    const float amp = bars75 ? 0.75f : 1.0f;

    QVector<VectorTarget> out;
    out.reserve(6);
    for (const auto& bar : kBarColors) {
        const Ycc v = rgbToYcc(k, bar.r * amp, bar.g * amp, bar.b * amp);
        out.push_back({QString::fromLatin1(bar.name), QPointF(double(v.cb), double(v.cr))});
    }
    return out;
}

QVector<LightningTarget> lightningTargets(bool bars75, Colorimetry c)
{
    const Coeffs k = coeffsFor(c);
    const float amp = bars75 ? 0.75f : 1.0f;

    QVector<LightningTarget> out;
    out.reserve(7);
    // White apex plus the six bar colors.
    out.push_back({QStringLiteral("W"), QPointF(0.0, double(amp)), QPointF(0.0, double(amp))});
    for (const auto& bar : kBarColors) {
        const Ycc v = rgbToYcc(k, bar.r * amp, bar.g * amp, bar.b * amp);
        out.push_back({QString::fromLatin1(bar.name),
                       QPointF(double(v.cb), double(v.y)),
                       QPointF(double(v.cr), double(v.y))});
    }
    return out;
}

} // namespace ColorMatrix
