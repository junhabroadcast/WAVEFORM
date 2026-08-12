#include "capture/DeckLinkCapture.h"
#include "video/V210Unpack.h"

#include <QMetaObject>
#include <QThread>
#include <QTimer>
#include <cmath>
#include <cstring>

#include "DeckLinkImport.h"

namespace {

constexpr long kBmdVideoInputEnableFormatDetection = 1; // bmdVideoInputEnableFormatDetection
constexpr long kBmdFrameHasNoInputSource = static_cast<long>(0x80000000);

// Generate SMPTE 75% color bars in 10-bit legal range planar YCbCr (BT.709).
VideoFramePtr makeColorBars(int width, int height, int64_t frameNumber)
{
    auto frame = std::make_shared<VideoFrame>();
    frame->width = width;
    frame->height = height;
    frame->frameNumber = frameNumber;
    frame->pixelFormat = PixelFormat::V210;
    frame->modeName = QStringLiteral("SIM 1080p59.94 bars").toStdString();
    frame->frameRate = 60000.0 / 1001.0;
    frame->interlaced = false;
    frame->hasSignal = true;

    const size_t n = size_t(width) * size_t(height);
    frame->y.assign(n, 64);
    frame->cb.assign(n, 512);
    frame->cr.assign(n, 512);

    // Y,Cb,Cr for 75% bars (approx BT.709 studio range 10-bit)
    struct Bar {
        uint16_t y, cb, cr;
    };
    const Bar bars[8] = {
        {721, 512, 512}, // white 75%
        {674, 176, 543}, // yellow
        {581, 589, 176}, // cyan
        {534, 253, 207}, // green
        {251, 771, 817}, // magenta
        {204, 435, 848}, // red
        {111, 848, 481}, // blue
        {64, 512, 512},  // black
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int idx = std::min(7, x * 8 / width);
            const size_t p = size_t(y) * size_t(width) + size_t(x);
            frame->y[p] = bars[idx].y;
            frame->cb[p] = bars[idx].cb;
            frame->cr[p] = bars[idx].cr;
        }
    }
    return frame;
}

QString bstrToQString(BSTR b)
{
    if (!b)
        return {};
    return QString::fromWCharArray(b);
}

} // namespace

class DeckLinkCapture::Callback : public IDeckLinkInputCallback {
public:
    explicit Callback(DeckLinkCapture* owner)
        : owner_(owner)
    {
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) override
    {
        if (!ppv)
            return E_POINTER;
        *ppv = nullptr;
        if (iid == IID_IUnknown || iid == IID_IDeckLinkInputCallback) {
            *ppv = static_cast<IDeckLinkInputCallback*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ref_.fetch_add(1) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG v = ref_.fetch_sub(1) - 1;
        if (v == 0)
            delete this;
        return v;
    }

    // MSVC #import exposes abstract methods as raw_* wrappers.
    HRESULT __stdcall raw_VideoInputFormatChanged(
        _BMDVideoInputFormatChangedEvents /*notificationEvents*/,
        IDeckLinkDisplayMode* newDisplayMode,
        _BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/) override
    {
        if (!newDisplayMode || !owner_)
            return S_OK;

        BSTR nameBstr = nullptr;
        newDisplayMode->GetName(&nameBstr);
        const QString name = bstrToQString(nameBstr);
        if (nameBstr)
            SysFreeString(nameBstr);

        const int width = int(newDisplayMode->GetWidth());
        const int height = int(newDisplayMode->GetHeight());
        __int64 duration = 0;
        __int64 scale = 0;
        newDisplayMode->GetFrameRate(&duration, &scale);
        double fps = 0.0;
        if (duration > 0 && scale > 0)
            fps = double(scale) / double(duration);

        const bool interlaced = newDisplayMode->GetFieldDominance() != bmdProgressiveFrame;

        IDeckLinkInput* input = reinterpret_cast<IDeckLinkInput*>(owner_->deckLinkInput_);
        if (input) {
            input->StopStreams();
            const auto mode = newDisplayMode->GetDisplayMode();
            HRESULT hr = input->EnableVideoInput(mode, bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection);
            if (FAILED(hr))
                hr = input->EnableVideoInput(mode, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection);
            if (SUCCEEDED(hr))
                input->StartStreams();
        }

        owner_->handleFormatChanged(name, width, height, fps, interlaced);
        return S_OK;
    }

    HRESULT __stdcall raw_VideoInputFrameArrived(
        IDeckLinkVideoInputFrame* videoFrame,
        IDeckLinkAudioInputPacket* /*audioPacket*/) override
    {
        if (!videoFrame || !owner_)
            return S_OK;

        void* bytes = nullptr;
        if (FAILED(videoFrame->GetBytes(&bytes)) || !bytes)
            return S_OK;

        const int width = int(videoFrame->GetWidth());
        const int height = int(videoFrame->GetHeight());
        const int rowBytes = int(videoFrame->GetRowBytes());
        const int pixelFormat = int(videoFrame->GetPixelFormat());
        const bool hasSignal = (videoFrame->GetFlags() & kBmdFrameHasNoInputSource) == 0;

        owner_->handleFrame(static_cast<const uint8_t*>(bytes), rowBytes, width, height, pixelFormat, hasSignal);
        return S_OK;
    }

private:
    DeckLinkCapture* owner_ = nullptr;
    std::atomic<ULONG> ref_{1};
};

DeckLinkCapture::DeckLinkCapture(FrameQueue& queue, QObject* parent)
    : QObject(parent)
    , queue_(queue)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

DeckLinkCapture::~DeckLinkCapture()
{
    stop();
    CoUninitialize();
}

QStringList DeckLinkCapture::listDevices()
{
    QStringList names;
    IDeckLinkIterator* iterator = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_CDeckLinkIterator, nullptr, CLSCTX_ALL, IID_IDeckLinkIterator,
                                  reinterpret_cast<void**>(&iterator));
    if (FAILED(hr) || !iterator)
        return names;

    IDeckLink* deckLink = nullptr;
    int index = 0;
    while (iterator->Next(&deckLink) == S_OK) {
        BSTR display = nullptr;
        deckLink->GetDisplayName(&display);
        names << (display ? bstrToQString(display) : QStringLiteral("DeckLink %1").arg(index));
        if (display)
            SysFreeString(display);
        deckLink->Release();
        ++index;
    }
    iterator->Release();
    return names;
}

void DeckLinkCapture::setForceColorBars(bool enabled)
{
    if (forceColorBars_ == enabled)
        return;
    forceColorBars_ = enabled;

    if (!running_) {
        emit statusChanged();
        return;
    }

    // Hot-switch between DeckLink and color-bar simulator while running.
    const int deviceIndex = lastDeviceIndex_;
    stop();
    start(deviceIndex);
}

bool DeckLinkCapture::start(int deviceIndex)
{
    stop();
    lastDeviceIndex_ = deviceIndex;

    // Reset detected format so stale info (e.g. "SIM ..." from a previous
    // simulator run) never leaks into the readout of a new capture session.
    modeName_.clear();
    width_ = 0;
    height_ = 0;
    fps_ = 0.0;
    interlaced_ = false;

    if (forceColorBars_) {
        startSimulator();
        running_ = true;
        emit statusChanged();
        return true;
    }

    if (!openDevice(deviceIndex)) {
        if (simulatorFallback_) {
            startSimulator();
            running_ = true;
            emit statusChanged();
            return true;
        }
        return false;
    }

    IDeckLinkInput* input = reinterpret_cast<IDeckLinkInput*>(deckLinkInput_);
    callback_ = new Callback(this);
    input->SetCallback(callback_);

    // Start with format detection; pick a common HD mode as initial enable.
    HRESULT hr = input->EnableVideoInput(bmdModeHD1080i5994, bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection);
    if (FAILED(hr))
        hr = input->EnableVideoInput(bmdModeHD1080i5994, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection);
    if (FAILED(hr))
        hr = input->EnableVideoInput(bmdModeHD1080p5994, bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection);
    if (FAILED(hr)) {
        closeDevice();
        if (simulatorFallback_) {
            startSimulator();
            running_ = true;
            emit statusChanged();
            return true;
        }
        return false;
    }

    hr = input->StartStreams();
    if (FAILED(hr)) {
        closeDevice();
        return false;
    }

    running_ = true;
    emit statusChanged();
    return true;
}

void DeckLinkCapture::stop()
{
    stopSimulator();
    if (deckLinkInput_) {
        IDeckLinkInput* input = reinterpret_cast<IDeckLinkInput*>(deckLinkInput_);
        input->StopStreams();
        input->DisableVideoInput();
        input->SetCallback(nullptr);
    }
    closeDevice();
    running_ = false;
    signalLocked_ = false;
    emit statusChanged();
}

bool DeckLinkCapture::openDevice(int deviceIndex)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    IDeckLinkIterator* iterator = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_CDeckLinkIterator, nullptr, CLSCTX_ALL, IID_IDeckLinkIterator,
                                  reinterpret_cast<void**>(&iterator));
    if (FAILED(hr) || !iterator)
        return false;

    IDeckLink* deckLink = nullptr;
    int index = 0;
    bool found = false;
    while (iterator->Next(&deckLink) == S_OK) {
        if (index == deviceIndex) {
            found = true;
            break;
        }
        deckLink->Release();
        deckLink = nullptr;
        ++index;
    }
    iterator->Release();

    if (!found || !deckLink)
        return false;

    IDeckLinkInput* input = nullptr;
    hr = deckLink->QueryInterface(IID_IDeckLinkInput, reinterpret_cast<void**>(&input));
    if (FAILED(hr) || !input) {
        deckLink->Release();
        return false;
    }

    deckLink_ = deckLink;
    deckLinkInput_ = input;
    return true;
}

void DeckLinkCapture::closeDevice()
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (callback_) {
        callback_->Release();
        callback_ = nullptr;
    }
    if (deckLinkInput_) {
        reinterpret_cast<IDeckLinkInput*>(deckLinkInput_)->Release();
        deckLinkInput_ = nullptr;
    }
    if (deckLink_) {
        reinterpret_cast<IDeckLink*>(deckLink_)->Release();
        deckLink_ = nullptr;
    }
}

void DeckLinkCapture::handleFormatChanged(const QString& name, int width, int height, double fps, bool interlaced)
{
    // DeckLink callbacks arrive on a worker thread — hop to the object thread.
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, name, width, height, fps, interlaced]() {
                handleFormatChanged(name, width, height, fps, interlaced);
            },
            Qt::QueuedConnection);
        return;
    }

    modeName_ = name;
    width_ = width;
    height_ = height;
    fps_ = fps;
    interlaced_ = interlaced;
    emit formatChanged(name, width, height, fps);
    emit statusChanged();
}

void DeckLinkCapture::handleFrame(const uint8_t* bytes, int rowBytes, int width, int height,
                                  int pixelFormatFourCC, bool hasSignal)
{
    if (!hasSignal) {
        noSignalFrames_.fetch_add(1);
        signalLocked_ = false;
        return;
    }

    auto frame = std::make_shared<VideoFrame>();
    frame->frameNumber = frameNumber_.fetch_add(1) + 1;
    frame->modeName = modeName_.toStdString();
    frame->frameRate = fps_;
    frame->interlaced = interlaced_;
    frame->hasSignal = true;

    bool ok = false;
    if (pixelFormatFourCC == int(bmdFormat10BitYUV))
        ok = V210Unpack::unpackV210(bytes, rowBytes, width, height, *frame);
    else if (pixelFormatFourCC == int(bmdFormat8BitYUV))
        ok = V210Unpack::unpackUYVY(bytes, rowBytes, width, height, *frame);

    if (!ok)
        return;

    if (width_ == 0) {
        handleFormatChanged(modeName_.isEmpty() ? QStringLiteral("SDI") : modeName_, width, height, fps_, interlaced_);
    }

    signalLocked_ = true;
    framesCaptured_.fetch_add(1);
    queue_.push(std::move(frame));
}

QString DeckLinkCapture::statusText() const
{
    if (simulatorActive_) {
        if (forceColorBars_)
            return QStringLiteral("COLOR BARS (forced) — 75% SMPTE bars");
        return QStringLiteral("SIMULATOR (no DeckLink input) — 75% color bars");
    }
    if (!running_)
        return QStringLiteral("Stopped");
    if (!signalLocked_)
        return QStringLiteral("Waiting for SDI lock…");
    return QStringLiteral("LOCKED %1  %2x%3  %4 fps")
        .arg(modeName_)
        .arg(width_)
        .arg(height_)
        .arg(fps_, 0, 'f', 2);
}

QString DeckLinkCapture::modeName() const
{
    return modeName_;
}

void DeckLinkCapture::startSimulator()
{
    simulatorActive_ = true;
    signalLocked_ = true;
    modeName_ = QStringLiteral("SIM 1080p59.94");
    width_ = 1920;
    height_ = 1080;
    fps_ = 60000.0 / 1001.0;
    interlaced_ = false;
    emit formatChanged(modeName_, width_, height_, fps_);

    if (!simulatorTimer_) {
        simulatorTimer_ = new QTimer(this);
        connect(simulatorTimer_, &QTimer::timeout, this, &DeckLinkCapture::simulatorTick);
    }
    simulatorTimer_->start(17); // ~60 Hz
}

void DeckLinkCapture::stopSimulator()
{
    if (simulatorTimer_)
        simulatorTimer_->stop();
    simulatorActive_ = false;
}

void DeckLinkCapture::simulatorTick()
{
    const int64_t n = frameNumber_.fetch_add(1) + 1;
    auto frame = makeColorBars(1920, 1080, n);
    framesCaptured_.fetch_add(1);
    queue_.push(std::move(frame));
}
