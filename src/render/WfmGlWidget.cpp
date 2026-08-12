#include "render/WfmGlWidget.h"
#include "color/ColorMatrix.h"
#include "display/Graticule.h"

#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

GLuint compileShader(QOpenGLFunctions_3_3_Core* gl, GLenum type, const QByteArray& src)
{
    GLuint s = gl->glCreateShader(type);
    const char* p = src.constData();
    GLint len = src.size();
    gl->glShaderSource(s, 1, &p, &len);
    gl->glCompileShader(s);
    GLint ok = 0;
    gl->glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl->glGetShaderInfoLog(s, 1024, nullptr, log);
        qWarning("Shader compile error: %s", log);
    }
    return s;
}

} // namespace

WfmGlWidget::WfmGlWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(320, 240);
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(0);
    fmt.setStencilBufferSize(0);
    fmt.setSamples(0);
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(fmt);

    // Status text as a real widget — avoids QPainter-on-GL glyph ghosting.
    readout_ = new QLabel(this);
    readout_->setAttribute(Qt::WA_TransparentForMouseEvents);
    readout_->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background-color: rgba(0, 0, 0, 210);"
        "  color: rgb(180, 255, 180);"
        "  font-family: Consolas, 'Courier New', monospace;"
        "  font-size: 12px;"
        "  padding-left: 8px;"
        "  padding-right: 8px;"
        "}"));
    readout_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    updateReadoutLabel();
}

WfmGlWidget::~WfmGlWidget()
{
    makeCurrent();
    if (glReady_) {
        glDeleteProgram(progPoints_);
        glDeleteProgram(progTonemap_);
        glDeleteProgram(progDecay_);
        glDeleteProgram(progPicture_);
        glDeleteVertexArrays(1, &vaoEmpty_);
        glDeleteTextures(1, &texY_);
        glDeleteTextures(1, &texCb_);
        glDeleteTextures(1, &texCr_);
        glDeleteTextures(1, &accumTex_);
        glDeleteTextures(1, &decayTex_);
        glDeleteFramebuffers(1, &accumFbo_);
        glDeleteFramebuffers(1, &decayFbo_);
    }
    doneCurrent();
}

void WfmGlWidget::setTileState(const TileState& state)
{
    const bool modeChanged = state.mode != state_.mode;
    state_ = state;
    setCursor(state_.mode == WfmDisplayMode::Video ? Qt::CrossCursor : Qt::ArrowCursor);
    if (modeChanged && glReady_ && accumFbo_) {
        makeCurrent();
        glBindFramebuffer(GL_FRAMEBUFFER, accumFbo_);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
        doneCurrent();
    }
    updateReadoutLabel();
    update();
}

void WfmGlWidget::setFrame(const VideoFramePtr& frame)
{
    if (!frame)
        return;
    if (state_.freeze) {
        if (!frozenFrame_)
            frozenFrame_ = frame;
    } else {
        frozenFrame_.reset();
        liveFrame_ = frame;
    }
    update();
}

void WfmGlWidget::setStatus(const QString& modeName, bool locked, uint64_t drops)
{
    if (modeName_ == modeName && locked_ == locked && drops_ == drops)
        return;
    modeName_ = modeName;
    locked_ = locked;
    drops_ = drops;
    updateReadoutLabel();
}

void WfmGlWidget::setMarkerLine(int line)
{
    if (markerLine_ == line)
        return;
    markerLine_ = line;
    updateReadoutLabel();
    update();
}

void WfmGlWidget::updateReadoutLabel()
{
    if (!readout_)
        return;

    const QString gain = state_.varGainEnabled
        ? QStringLiteral("VAR %1x").arg(state_.varGain, 0, 'f', 2)
        : QStringLiteral("%1x").arg(state_.gain, 0, 'f', 0);
    QString line = QStringLiteral("%1 | Gain %2 | Mag %3x | %4 | drops %5")
                       .arg(displayModeName(state_.mode))
                       .arg(gain)
                       .arg(state_.mag, 0, 'f', 0)
                       .arg(modeName_.isEmpty() ? QStringLiteral("-") : modeName_)
                       .arg(drops_);
    if (state_.mode == WfmDisplayMode::Video) {
        if (state_.lineSelectEnabled)
            line += QStringLiteral(" | LINE %1").arg(state_.selectedLine);
        else if (markerLine_ >= 0)
            line += QStringLiteral(" | LINE %1").arg(markerLine_);
    }
    if (state_.freeze)
        line += QStringLiteral(" | FREEZE");
    if (!locked_)
        line += QStringLiteral(" | NO LOCK");
    readout_->setText(line);
}

void WfmGlWidget::layoutReadoutLabel()
{
    if (!readout_)
        return;
    constexpr int kH = 22;
    readout_->setGeometry(0, height() - kH, width(), kH);
    readout_->raise();
}

void WfmGlWidget::resizeEvent(QResizeEvent* event)
{
    QOpenGLWidget::resizeEvent(event);
    layoutReadoutLabel();
}

int WfmGlWidget::lineAtPosition(const QPointF& pos) const
{
    const VideoFramePtr frame = state_.freeze && frozenFrame_ ? frozenFrame_ : liveFrame_;
    if (!frame || frame->empty() || width() <= 0 || height() <= 0)
        return -1;

    // Invert the letterbox/pillarbox mapping used by picture.frag.
    const float frameAspect = float(frame->width) / float(std::max(frame->height, 1));
    const float viewAspect = float(width()) / float(height());
    float u = float(pos.x()) / float(width());
    float v = float(pos.y()) / float(height()); // 0 at top, like the unpacked planes

    if (viewAspect > frameAspect) {
        const float w = frameAspect / viewAspect;
        u = (u - 0.5f) / w + 0.5f;
    } else {
        const float h = viewAspect / frameAspect;
        v = (v - 0.5f) / h + 0.5f;
    }

    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
        return -1;
    return std::clamp(int(v * float(frame->height)), 0, frame->height - 1);
}

void WfmGlWidget::mousePressEvent(QMouseEvent* event)
{
    if (state_.mode == WfmDisplayMode::Video && event->button() == Qt::LeftButton) {
        const int line = lineAtPosition(event->position());
        if (line >= 0)
            emit lineClicked(line);
    }
    QOpenGLWidget::mousePressEvent(event);
}

void WfmGlWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (state_.mode == WfmDisplayMode::Video && (event->buttons() & Qt::LeftButton)) {
        const int line = lineAtPosition(event->position());
        if (line >= 0)
            emit lineClicked(line);
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

bool WfmGlWidget::loadShaderProgram(unsigned& program, const char* vertRes, const char* fragRes)
{
    QFile vf(QString::fromLatin1(vertRes));
    QFile ff(QString::fromLatin1(fragRes));
    if (!vf.open(QIODevice::ReadOnly) || !ff.open(QIODevice::ReadOnly))
        return false;
    GLuint vs = compileShader(this, GL_VERTEX_SHADER, vf.readAll());
    GLuint fs = compileShader(this, GL_FRAGMENT_SHADER, ff.readAll());
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return ok == GL_TRUE;
}

void WfmGlWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glReady_ = true;

    loadShaderProgram(progPoints_, ":/shaders/shaders/points.vert", ":/shaders/shaders/points.frag");
    loadShaderProgram(progTonemap_, ":/shaders/shaders/tonemap.vert", ":/shaders/shaders/tonemap.frag");
    loadShaderProgram(progDecay_, ":/shaders/shaders/tonemap.vert", ":/shaders/shaders/decay.frag");
    loadShaderProgram(progPicture_, ":/shaders/shaders/tonemap.vert", ":/shaders/shaders/picture.frag");

    glGenVertexArrays(1, &vaoEmpty_);

    glGenTextures(1, &texY_);
    glGenTextures(1, &texCb_);
    glGenTextures(1, &texCr_);
    for (GLuint t : {texY_, texCb_, texCr_}) {
        glBindTexture(GL_TEXTURE_2D, t);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenFramebuffers(1, &accumFbo_);
    glGenTextures(1, &accumTex_);
    glGenFramebuffers(1, &decayFbo_);
    glGenTextures(1, &decayTex_);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClearColor(0, 0, 0, 1);
}

void WfmGlWidget::ensureAccumSize()
{
    const int w = std::max(1, int(std::lround(width() * devicePixelRatioF())));
    const int h = std::max(1, int(std::lround(height() * devicePixelRatioF())));
    if (w == accumW_ && h == accumH_)
        return;
    accumW_ = w;
    accumH_ = h;

    auto alloc = [&](GLuint tex, GLuint fbo) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    };
    alloc(accumTex_, accumFbo_);
    alloc(decayTex_, decayFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glBindFramebuffer(GL_FRAMEBUFFER, accumFbo_);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void WfmGlWidget::resizeGL(int, int)
{
    ensureAccumSize();
}

void WfmGlWidget::uploadPlanes(const VideoFrame& frame)
{
    if (frame.width != texW_ || frame.height != texH_) {
        texW_ = frame.width;
        texH_ = frame.height;
        for (GLuint t : {texY_, texCb_, texCr_}) {
            glBindTexture(GL_TEXTURE_2D, t);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, texW_, texH_, 0, GL_RED, GL_UNSIGNED_SHORT, nullptr);
        }
    }

    auto up = [&](GLuint tex, const std::vector<uint16_t>& plane) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW_, texH_, GL_RED, GL_UNSIGNED_SHORT, plane.data());
    };
    up(texY_, frame.y);
    up(texCb_, frame.cb);
    up(texCr_, frame.cr);
}

void WfmGlWidget::decayAccum()
{
    glBindFramebuffer(GL_FRAMEBUFFER, decayFbo_);
    glViewport(0, 0, accumW_, accumH_);
    glDisable(GL_BLEND);
    glUseProgram(progDecay_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumTex_);
    glUniform1i(glGetUniformLocation(progDecay_, "uAccum"), 0);
    glUniform1f(glGetUniformLocation(progDecay_, "uDecay"), state_.persistence);
    glBindVertexArray(vaoEmpty_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Copy decay result back to accum
    glBindFramebuffer(GL_READ_FRAMEBUFFER, decayFbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, accumFbo_);
    glBlitFramebuffer(0, 0, accumW_, accumH_, 0, 0, accumW_, accumH_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, accumFbo_);
}

void WfmGlWidget::renderTrace()
{
    const VideoFramePtr frame = state_.freeze && frozenFrame_ ? frozenFrame_ : liveFrame_;
    if (!frame || frame->empty())
        return;

    uploadPlanes(*frame);

    glBindFramebuffer(GL_FRAMEBUFFER, accumFbo_);
    glViewport(0, 0, accumW_, accumH_);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glUseProgram(progPoints_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texY_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texCb_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texCr_);
    glUniform1i(glGetUniformLocation(progPoints_, "uY"), 0);
    glUniform1i(glGetUniformLocation(progPoints_, "uCb"), 1);
    glUniform1i(glGetUniformLocation(progPoints_, "uCr"), 2);
    glUniform2i(glGetUniformLocation(progPoints_, "uSize"), frame->width, frame->height);
    glUniform1i(glGetUniformLocation(progPoints_, "uMode"), int(state_.mode));
    glUniform1i(glGetUniformLocation(progPoints_, "uStyle"), int(state_.style));
    glUniform1i(glGetUniformLocation(progPoints_, "uSweep"), int(state_.sweep));
    glUniform1i(glGetUniformLocation(progPoints_, "uComponents"), int(state_.components));
    glUniform1f(glGetUniformLocation(progPoints_, "uGain"), state_.effectiveGain());
    glUniform1f(glGetUniformLocation(progPoints_, "uMag"), state_.mag);
    glUniform1i(glGetUniformLocation(progPoints_, "uLineSelect"),
                state_.lineSelectEnabled ? state_.selectedLine : -1);
    glUniform1f(glGetUniformLocation(progPoints_, "uAdd"), 0.08f);
    glUniform1f(glGetUniformLocation(progPoints_, "uPointSize"), 1.0f);

    // Trace must land exactly where the QPainter graticule draws its targets.
    // Mirror the geometry of drawGraticuleOverlay: full rect minus the 22 px
    // readout strip, converted to NDC over the whole widget.
    {
        const float W = float(width());
        const float H = float(height());
        const QRectF r = QRectF(0, 0, width(), height()).adjusted(0, 0, 0, -22);
        const QPointF c = r.center();
        const float cxNdc = float(c.x()) * 2.0f / W - 1.0f;
        const float cyNdc = 1.0f - float(c.y()) * 2.0f / H;
        float sx = 1.0f;
        float sy = 1.0f;
        if (state_.mode == WfmDisplayMode::Vector) {
            // Graticule: pixel offset = q * 2 * radius (q in -0.5..0.5).
            const float radius = float(qMin(r.width(), r.height()) * 0.42);
            sx = 4.0f * radius / W;
            sy = 4.0f * radius / H;
        } else if (state_.mode == WfmDisplayMode::Lightning) {
            // Graticule: x offset = q * 2 * halfW, y offset = Y * halfH.
            const float halfW = float(r.width() * 0.45);
            const float halfH = float(r.height() * 0.5);
            sx = 4.0f * halfW / W;
            sy = 2.0f * halfH / H;
        }
        glUniform2f(glGetUniformLocation(progPoints_, "uTraceCenter"), cxNdc, cyNdc);
        glUniform2f(glGetUniformLocation(progPoints_, "uTraceScale"), sx, sy);
    }

    const GLsizei count = state_.lineSelectEnabled ? frame->width : frame->width * frame->height;
    glBindVertexArray(vaoEmpty_);

    const GLint locLineMode = glGetUniformLocation(progPoints_, "uLineMode");
    const bool connectSamples = state_.mode != WfmDisplayMode::Waveform;

    auto drawPass = [&](int pass) {
        glUniform1i(glGetUniformLocation(progPoints_, "uComponentPass"), pass);
        if (connectSamples && count > 1) {
            // Connected segments between consecutive samples give the
            // continuous CRT-style trace (essential for sparse signals
            // like color bars on the vectorscope).
            glUniform1i(locLineMode, 1);
            glDrawArrays(GL_LINES, 0, (count - 1) * 2);
        }
        glUniform1i(locLineMode, 0);
        glDrawArrays(GL_POINTS, 0, count);
    };

    if (state_.mode == WfmDisplayMode::Waveform) {
        drawPass(0);
        drawPass(1);
        drawPass(2);
    } else if (state_.mode == WfmDisplayMode::Vector) {
        drawPass(0);
    } else if (state_.mode == WfmDisplayMode::Lightning) {
        drawPass(0); // Pb-Y
        drawPass(1); // Pr-Y
    }

    glDisable(GL_BLEND);
}

void WfmGlWidget::tonemapToScreen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, int(std::lround(width() * devicePixelRatioF())),
               int(std::lround(height() * devicePixelRatioF())));
    glDisable(GL_BLEND);
    glUseProgram(progTonemap_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumTex_);
    glUniform1i(glGetUniformLocation(progTonemap_, "uAccum"), 0);
    glUniform1f(glGetUniformLocation(progTonemap_, "uIntensity"), state_.intensity);
    glBindVertexArray(vaoEmpty_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void WfmGlWidget::drawGraticuleOverlay(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    // Leave bottom strip for the QLabel readout.
    const QRectF r = rect().adjusted(0, 0, 0, -22);
    const Colorimetry c = ColorMatrix::resolve(colorimetry_, liveFrame_ ? liveFrame_->height : 1080);

    switch (state_.mode) {
    case WfmDisplayMode::Waveform:
        Graticule::drawWaveform(painter, r, state_);
        break;
    case WfmDisplayMode::Vector:
        Graticule::drawVector(painter, r, state_, c);
        break;
    case WfmDisplayMode::Lightning:
        Graticule::drawLightning(painter, r, state_, c);
        break;
    case WfmDisplayMode::Video: {
        // Marker line + line number drawn over the picture.
        const int line = state_.lineSelectEnabled ? state_.selectedLine : markerLine_;
        const VideoFramePtr frame = state_.freeze && frozenFrame_ ? frozenFrame_ : liveFrame_;
        if (line >= 0 && frame && !frame->empty()) {
            const float W = float(width());
            const float H = float(height());
            const float frameAspect = float(frame->width) / float(std::max(frame->height, 1));
            const float viewAspect = W / H;

            // Forward letterbox/pillarbox mapping (same as picture.frag).
            float v = (float(line) + 0.5f) / float(frame->height);
            float x0 = 0.0f;
            float x1 = W;
            if (viewAspect > frameAspect) {
                const float w = frameAspect / viewAspect;
                x0 = (0.5f - 0.5f * w) * W;
                x1 = (0.5f + 0.5f * w) * W;
            } else {
                const float h = viewAspect / frameAspect;
                v = (v - 0.5f) * h + 0.5f;
            }
            const float y = v * H;

            painter.setPen(QPen(QColor(255, 216, 50, 230), 1.0));
            painter.drawLine(QPointF(x0, y), QPointF(x1, y));
        }
        break;
    }
    case WfmDisplayMode::None:
        break;
    }
}

void WfmGlWidget::renderPicture()
{
    const VideoFramePtr frame = state_.freeze && frozenFrame_ ? frozenFrame_ : liveFrame_;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, int(std::lround(width() * devicePixelRatioF())),
               int(std::lround(height() * devicePixelRatioF())));
    glDisable(GL_BLEND);
    glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!frame || frame->empty())
        return;

    uploadPlanes(*frame);

    const Colorimetry c = ColorMatrix::resolve(colorimetry_, frame->height);

    glUseProgram(progPicture_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texY_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texCb_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texCr_);
    glUniform1i(glGetUniformLocation(progPicture_, "uY"), 0);
    glUniform1i(glGetUniformLocation(progPicture_, "uCb"), 1);
    glUniform1i(glGetUniformLocation(progPicture_, "uCr"), 2);
    glUniform2f(glGetUniformLocation(progPicture_, "uFrameSize"), float(frame->width), float(frame->height));
    glUniform2f(glGetUniformLocation(progPicture_, "uViewportSize"),
                float(width() * devicePixelRatioF()), float(height() * devicePixelRatioF()));
    glUniform1i(glGetUniformLocation(progPicture_, "uColorimetry"), (c == Colorimetry::BT601) ? 0 : 1);
    // Own line select takes priority; otherwise show the line picked on a scope tile.
    const int highlightLine = state_.lineSelectEnabled ? state_.selectedLine : markerLine_;
    glUniform1i(glGetUniformLocation(progPicture_, "uLineSelect"), highlightLine);
    glBindVertexArray(vaoEmpty_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void WfmGlWidget::paintGL()
{
    if (state_.mode == WfmDisplayMode::None) {
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
        glViewport(0, 0, int(std::lround(width() * devicePixelRatioF())),
                   int(std::lround(height() * devicePixelRatioF())));
        glDisable(GL_BLEND);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    if (state_.mode == WfmDisplayMode::Video) {
        renderPicture();
    } else {
        ensureAccumSize();
        decayAccum();
        renderTrace();
        tonemapToScreen();
    }

    // Must paint overlays inside paintGL. Drawing in paintEvent leaves stale
    // text pixels (drops counter changes every frame → smeared glyphs).
    QPainter painter(this);
    drawGraticuleOverlay(painter);
}
