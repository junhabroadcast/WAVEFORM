#pragma once

#include "display/DisplayModes.h"
#include "video/VideoFrame.h"

#include <QPainter>
#include <QRectF>

namespace Graticule {

void drawWaveform(QPainter& p, const QRectF& r, const TileState& state);
void drawVector(QPainter& p, const QRectF& r, const TileState& state, Colorimetry colorimetry);
void drawLightning(QPainter& p, const QRectF& r, const TileState& state);

void drawReadouts(QPainter& p, const QRectF& r, const TileState& state, const QString& modeName,
                  bool locked, uint64_t drops);

} // namespace Graticule
