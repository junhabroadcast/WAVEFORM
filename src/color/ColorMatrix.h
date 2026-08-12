#pragma once

#include "video/VideoFrame.h"

#include <QPointF>
#include <QVector>
#include <utility>

namespace ColorMatrix {

Colorimetry resolve(Colorimetry selected, int height);

// Normalize 10-bit studio Y (64-940) to 0..1 voltage-ish (0 = black, 1 = 100% white ~700mV).
inline float normY(uint16_t y10)
{
    return (float(y10) - 64.0f) / 876.0f;
}

// Normalize 10-bit chroma (512 center, 64-960) to -0.5 .. +0.5
inline float normC(uint16_t c10)
{
    return (float(c10) - 512.0f) / 896.0f;
}

struct VectorTarget {
    QString name;
    QPointF pos; // Cb,Cr in -0.5..0.5
};

// Color bar targets in Cb/Cr space, computed exactly from the matrix.
QVector<VectorTarget> vectorTargets(bool bars75, Colorimetry c);

struct LightningTarget {
    QString name;
    QPointF upper; // Pb (-0.5..0.5) vs Y (0..1), top half
    QPointF lower; // Pr (-0.5..0.5) vs Y (0..1), bottom half
};

QVector<LightningTarget> lightningTargets(bool bars75, Colorimetry c);

} // namespace ColorMatrix
