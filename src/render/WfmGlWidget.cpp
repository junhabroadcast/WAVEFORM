#include "render/WfmGlWidget.h"
#include "color/ColorMatrix.h"
#include "display/Graticule.h"

#include <QFile>
#include <QPaintEvent>
#include <QPainter>
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
}

WfmGlWidget::~WfmGlWidget()
{
    makeCurrent();
    if (glReady_) {
        glDeleteProgram(progPoints_);
        glDeleteProgram(progTonemap_);
        glDeleteProgram(progDecay_);
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
    state_ = state;
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
    modeName_ = modeName;
    locked_ = locked;
    drops_ = drops;
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

    const GLsizei count = state_.lineSelectEnabled ? frame->width : frame->width * frame->height;
    glBindVertexArray(vaoEmpty_);

    auto drawPass = [&](int pass) {
        glUniform1i(glGetUniformLocation(progPoints_, "uComponentPass"), pass);
        glDrawArrays(GL_POINTS, 0, count);
    };

    if (state_.mode == WfmDisplayMode::Waveform) {
        drawPass(0);
        drawPass(1);
        drawPass(2);
    } else if (state_.mode == WfmDisplayMode::Vector) {
        drawPass(0);
    } else {
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

void WfmGlWidget::drawGraticuleOverlay()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = rect();
    const Colorimetry c = ColorMatrix::resolve(colorimetry_, liveFrame_ ? liveFrame_->height : 1080);

    switch (state_.mode) {
    case WfmDisplayMode::Waveform:
        Graticule::drawWaveform(painter, r, state_);
        break;
    case WfmDisplayMode::Vector:
        Graticule::drawVector(painter, r, state_, c);
        break;
    case WfmDisplayMode::Lightning:
        Graticule::drawLightning(painter, r, state_);
        break;
    }
    Graticule::drawReadouts(painter, r, state_, modeName_, locked_, drops_);
}

void WfmGlWidget::paintGL()
{
    ensureAccumSize();
    decayAccum();
    renderTrace();
    tonemapToScreen();
}

void WfmGlWidget::paintEvent(QPaintEvent* event)
{
    // Custom: run GL then overlay. QOpenGLWidget::paintEvent calls paintGL.
    QOpenGLWidget::paintEvent(event);
    drawGraticuleOverlay();
}
