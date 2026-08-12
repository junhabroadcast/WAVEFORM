#pragma once

#include "capture/FrameQueue.h"
#include "video/VideoFrame.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

struct DeckLinkDeviceInfo {
    int index = 0;
    QString displayName;
    QString modelName;
};

class DeckLinkCapture : public QObject {
    Q_OBJECT
public:
    explicit DeckLinkCapture(FrameQueue& queue, QObject* parent = nullptr);
    ~DeckLinkCapture() override;

    QStringList listDevices();
    bool start(int deviceIndex = 0);
    void stop();
    bool isRunning() const { return running_.load(); }

    QString statusText() const;
    QString modeName() const;
    bool signalLocked() const { return signalLocked_.load(); }
    uint64_t framesCaptured() const { return framesCaptured_.load(); }
    uint64_t framesDroppedNoSignal() const { return noSignalFrames_.load(); }

    // When true and no DeckLink device, synthesize 75% color bars for offline validation.
    void setSimulatorFallback(bool enabled) { simulatorFallback_ = enabled; }
    bool simulatorActive() const { return simulatorActive_.load(); }

signals:
    void statusChanged();
    void formatChanged(QString modeName, int width, int height, double fps);

private:
    class Callback;
    friend class Callback;

    bool openDevice(int deviceIndex);
    void closeDevice();
    void handleFormatChanged(const QString& name, int width, int height, double fps, bool interlaced);
    void handleFrame(const uint8_t* bytes, int rowBytes, int width, int height,
                     int pixelFormatFourCC, bool hasSignal);
    void startSimulator();
    void stopSimulator();
    void simulatorTick();

    FrameQueue& queue_;
    std::mutex apiMutex_;

    // Opaque COM pointers held as void* to keep header free of DeckLink types.
    void* deckLink_ = nullptr;
    void* deckLinkInput_ = nullptr;
    Callback* callback_ = nullptr;

    std::atomic<bool> running_{false};
    std::atomic<bool> signalLocked_{false};
    std::atomic<bool> simulatorActive_{false};
    std::atomic<uint64_t> framesCaptured_{0};
    std::atomic<uint64_t> noSignalFrames_{0};
    std::atomic<int64_t> frameNumber_{0};

    QString modeName_;
    int width_ = 0;
    int height_ = 0;
    double fps_ = 0.0;
    bool interlaced_ = false;
    bool simulatorFallback_ = true;

    class QTimer* simulatorTimer_ = nullptr;
};
