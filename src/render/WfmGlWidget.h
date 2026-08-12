#pragma once

#include "display/DisplayModes.h"
#include "video/VideoFrame.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QString>

class QLabel;
class QPainter;
class QResizeEvent;

class WfmGlWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit WfmGlWidget(QWidget* parent = nullptr);
    ~WfmGlWidget() override;

    void setTileState(const TileState& state);
    TileState tileState() const { return state_; }

    void setFrame(const VideoFramePtr& frame);
    void setStatus(const QString& modeName, bool locked, uint64_t drops);
    void setColorimetry(Colorimetry c) { colorimetry_ = c; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    bool loadShaderProgram(unsigned& program, const char* vertRes, const char* fragRes);
    void ensureAccumSize();
    void uploadPlanes(const VideoFrame& frame);
    void renderTrace();
    void renderPicture();
    void decayAccum();
    void tonemapToScreen();
    void drawGraticuleOverlay(QPainter& painter);
    void updateReadoutLabel();
    void layoutReadoutLabel();

    TileState state_;
    VideoFramePtr liveFrame_;
    VideoFramePtr frozenFrame_;
    QString modeName_;
    bool locked_ = false;
    uint64_t drops_ = 0;
    Colorimetry colorimetry_ = Colorimetry::Auto;

    QLabel* readout_ = nullptr;

    GLuint progPoints_ = 0;
    GLuint progTonemap_ = 0;
    GLuint progDecay_ = 0;
    GLuint progPicture_ = 0;
    GLuint vaoEmpty_ = 0;

    GLuint texY_ = 0;
    GLuint texCb_ = 0;
    GLuint texCr_ = 0;
    int texW_ = 0;
    int texH_ = 0;

    GLuint accumFbo_ = 0;
    GLuint accumTex_ = 0;
    GLuint decayFbo_ = 0;
    GLuint decayTex_ = 0;
    int accumW_ = 0;
    int accumH_ = 0;

    bool glReady_ = false;
};
