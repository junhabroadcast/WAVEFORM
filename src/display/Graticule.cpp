#include "display/Graticule.h"
#include "color/ColorMatrix.h"

#include <QtMath>

namespace Graticule {
namespace {

QPen thinPen(const QColor& c = QColor(80, 180, 80, 160))
{
    QPen pen(c);
    pen.setWidthF(1.0);
    return pen;
}

} // namespace

void drawWaveform(QPainter& p, const QRectF& r, const TileState& state)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(thinPen());

    // Horizontal voltage lines: 0, 50%, 100% (0 / 350 / 700 mV-ish)
    const QStringList labels = {QStringLiteral("100"), QStringLiteral("50"), QStringLiteral("0")};
    for (int i = 0; i < 3; ++i) {
        const qreal y = r.top() + r.height() * (0.08 + 0.84 * (i / 2.0));
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        p.setPen(QColor(140, 220, 140));
        p.drawText(QPointF(r.left() + 4, y - 2), labels[i] + QStringLiteral("%"));
        p.setPen(thinPen());
    }

    // Vertical time divisions
    const int divisions = state.style == WaveformStyle::Parade ? 12 : 10;
    for (int i = 0; i <= divisions; ++i) {
        const qreal x = r.left() + r.width() * (i / double(divisions));
        p.drawLine(QPointF(x, r.top() + r.height() * 0.08), QPointF(x, r.bottom() - r.height() * 0.08));
    }

    if (state.style == WaveformStyle::Parade) {
        p.setPen(QColor(200, 220, 120));
        const qreal w = r.width() / 3.0;
        p.drawText(QPointF(r.left() + w * 0.45, r.top() + 14), QStringLiteral("Y"));
        p.drawText(QPointF(r.left() + w * 1.45, r.top() + 14), QStringLiteral("Cb"));
        p.drawText(QPointF(r.left() + w * 2.45, r.top() + 14), QStringLiteral("Cr"));
    }

    p.restore();
}

void drawVector(QPainter& p, const QRectF& r, const TileState& state, Colorimetry colorimetry)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    const QPointF c = r.center();
    const qreal radius = qMin(r.width(), r.height()) * 0.42;

    p.setPen(thinPen(QColor(60, 140, 60, 180)));
    p.drawEllipse(c, radius, radius);
    p.drawLine(QPointF(c.x() - radius, c.y()), QPointF(c.x() + radius, c.y()));
    p.drawLine(QPointF(c.x(), c.y() - radius), QPointF(c.x(), c.y() + radius));

    // I/Q-ish axes at 33 degrees
    const qreal ang = qDegreesToRadians(33.0);
    p.drawLine(c + QPointF(qCos(ang) * radius, -qSin(ang) * radius),
               c - QPointF(qCos(ang) * radius, -qSin(ang) * radius));
    p.drawLine(c + QPointF(qCos(ang + M_PI_2) * radius, -qSin(ang + M_PI_2) * radius),
               c - QPointF(qCos(ang + M_PI_2) * radius, -qSin(ang + M_PI_2) * radius));

    const auto targets = ColorMatrix::vectorTargets(state.bars75, colorimetry);
    p.setPen(QColor(220, 220, 80));
    p.setBrush(Qt::NoBrush);
    const qreal box = radius * 0.06;
    for (const auto& t : targets) {
        const QPointF pt(c.x() + t.pos.x() * 2.0 * radius, c.y() - t.pos.y() * 2.0 * radius);
        p.drawRect(QRectF(pt.x() - box, pt.y() - box, box * 2, box * 2));
        p.drawText(pt + QPointF(box + 2, -2), t.name);
    }

    p.setPen(QColor(160, 220, 160));
    p.drawText(r.adjusted(6, 4, 0, 0).topLeft() + QPointF(0, 12),
               state.bars75 ? QStringLiteral("75% TARGETS") : QStringLiteral("100% TARGETS"));
    p.restore();
}

void drawLightning(QPainter& p, const QRectF& r, const TileState& state)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(thinPen());

    const qreal midY = r.center().y();
    p.drawLine(QPointF(r.left(), midY), QPointF(r.right(), midY));
    p.drawLine(QPointF(r.center().x(), r.top()), QPointF(r.center().x(), r.bottom()));

    // Vertical Y scale marks
    for (int i = 0; i <= 4; ++i) {
        const qreal yu = r.top() + (midY - r.top()) * (i / 4.0);
        const qreal yl = midY + (r.bottom() - midY) * (i / 4.0);
        p.drawLine(QPointF(r.center().x() - 4, yu), QPointF(r.center().x() + 4, yu));
        p.drawLine(QPointF(r.center().x() - 4, yl), QPointF(r.center().x() + 4, yl));
    }

    const auto targets = ColorMatrix::lightningTargets(state.bars75);
    p.setPen(QColor(220, 220, 80));
    const qreal halfH = (midY - r.top());
    const qreal halfW = r.width() * 0.45;
    for (const auto& t : targets) {
        const QPointF up(r.center().x() + t.upper.x() * 2.0 * halfW,
                         midY - t.upper.y() * halfH);
        const QPointF lo(r.center().x() + t.lower.x() * 2.0 * halfW,
                         midY + (1.0 - t.lower.y()) * halfH);
        p.drawRect(QRectF(up.x() - 3, up.y() - 3, 6, 6));
        p.drawRect(QRectF(lo.x() - 3, lo.y() - 3, 6, 6));
    }

    p.setPen(QColor(160, 220, 160));
    p.drawText(QPointF(r.left() + 6, r.top() + 14), QStringLiteral("Pb / Y"));
    p.drawText(QPointF(r.left() + 6, r.bottom() - 8), QStringLiteral("Pr / Y"));
    p.restore();
}

void drawReadouts(QPainter& p, const QRectF& r, const TileState& state, const QString& modeName,
                  bool locked, uint64_t drops)
{
    p.save();
    QFont f = p.font();
    f.setFamily(QStringLiteral("Consolas"));
    f.setPixelSize(12);
    p.setFont(f);

    const QString gain = state.varGainEnabled
        ? QStringLiteral("VAR %1x").arg(state.varGain, 0, 'f', 2)
        : QStringLiteral("%1x").arg(state.gain, 0, 'f', 0);
    const QString line = QStringLiteral("%1 | Gain %2 | Mag %3x | %4 | drops %5%6%7")
                             .arg(displayModeName(state.mode))
                             .arg(gain)
                             .arg(state.mag, 0, 'f', 0)
                             .arg(modeName)
                             .arg(drops)
                             .arg(state.freeze ? QStringLiteral(" | FREEZE") : QString())
                             .arg(locked ? QString() : QStringLiteral(" | NO LOCK"));

    // Opaque strip so changing counters never leave glyph ghosts.
    const QRectF band(r.left(), r.bottom() - 22, r.width(), 22);
    p.fillRect(band, QColor(0, 0, 0, 200));
    p.setPen(QColor(180, 255, 180));
    p.drawText(band.adjusted(8, 0, -8, -2), Qt::AlignVCenter | Qt::AlignLeft, line);
    p.restore();
}

} // namespace Graticule
