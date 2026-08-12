#pragma once

#include <QString>

enum class WfmDisplayMode {
    Waveform,
    Vector,
    Lightning,
    Video,
    None,
};

enum class WaveformStyle {
    Overlay,
    Parade,
};

enum class SweepMode {
    Line,
    Field,
};

enum class ComponentFlags : int {
    None = 0,
    Y = 1 << 0,
    Cb = 1 << 1,
    Cr = 1 << 2,
    All = Y | Cb | Cr,
};

inline ComponentFlags operator|(ComponentFlags a, ComponentFlags b)
{
    return ComponentFlags(int(a) | int(b));
}
inline ComponentFlags operator&(ComponentFlags a, ComponentFlags b)
{
    return ComponentFlags(int(a) & int(b));
}
inline bool hasFlag(ComponentFlags f, ComponentFlags bit)
{
    return (int(f) & int(bit)) != 0;
}

struct TileState {
    WfmDisplayMode mode = WfmDisplayMode::Waveform;
    WaveformStyle style = WaveformStyle::Parade;
    SweepMode sweep = SweepMode::Line;
    ComponentFlags components = ComponentFlags::All;
    float gain = 1.0f;
    float varGain = 1.0f;
    bool varGainEnabled = false;
    float mag = 1.0f;
    bool lineSelectEnabled = false;
    int selectedLine = 0; // 0-based
    bool freeze = false;
    bool bars75 = true;
    float persistence = 0.0f; // decay factor per frame (0 = no trail)
    float intensity = 0.35f;

    float effectiveGain() const { return varGainEnabled ? varGain : gain; }
};

inline QString displayModeName(WfmDisplayMode m)
{
    switch (m) {
    case WfmDisplayMode::Waveform:
        return QStringLiteral("WAVEFORM");
    case WfmDisplayMode::Vector:
        return QStringLiteral("VECTOR");
    case WfmDisplayMode::Lightning:
        return QStringLiteral("LIGHTNING");
    case WfmDisplayMode::Video:
        return QStringLiteral("VIDEO");
    case WfmDisplayMode::None:
        return QStringLiteral("NONE");
    }
    return {};
}
