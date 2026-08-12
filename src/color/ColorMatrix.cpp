#include "color/ColorMatrix.h"

namespace ColorMatrix {

Colorimetry resolve(Colorimetry selected, int height)
{
    if (selected != Colorimetry::Auto)
        return selected;
    return height >= 720 ? Colorimetry::BT709 : Colorimetry::BT601;
}

QVector<VectorTarget> vectorTargets(bool bars75, Colorimetry /*c*/)
{
    // Approximate Rec.709 color bar locations in normalized Cb/Cr.
    // Values tuned for studio legal-range bars.
    const float s = bars75 ? 0.75f : 1.0f;
    auto t = [&](const char* name, float cb, float cr) {
        return VectorTarget{QString::fromLatin1(name), QPointF(double(cb * s), double(cr * s))};
    };

    return {
        t("Mg", 0.33f, 0.42f),
        t("R", -0.10f, 0.45f),
        t("Yl", -0.42f, 0.08f),
        t("G", -0.34f, -0.42f),
        t("Cy", 0.10f, -0.45f),
        t("B", 0.42f, -0.08f),
    };
}

QVector<LightningTarget> lightningTargets(bool bars75)
{
    const float s = bars75 ? 0.75f : 1.0f;
    // Simplified green-magenta transition targets along lightning scale.
    struct Row {
        float y;
        float pb;
        float pr;
    };
    const Row rows[] = {
        {0.00f, 0.00f, 0.00f},
        {0.15f, 0.25f * s, -0.20f * s},
        {0.35f, -0.10f * s, 0.30f * s},
        {0.55f, 0.20f * s, -0.25f * s},
        {0.75f, -0.30f * s, 0.15f * s},
        {1.00f, 0.00f, 0.00f},
    };

    QVector<LightningTarget> out;
    out.reserve(6);
    for (const auto& r : rows)
        out.push_back({QPointF(double(r.pb), double(r.y)), QPointF(double(r.pr), double(r.y))});
    return out;
}

} // namespace ColorMatrix
