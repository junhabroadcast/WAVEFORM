#pragma once

#include "capture/DeckLinkCapture.h"
#include "capture/FrameQueue.h"
#include "display/DisplayModes.h"

#include <QMainWindow>
#include <array>
#include <memory>

class WfmGlWidget;
class TileControls;
class QLabel;
class QComboBox;
class QCheckBox;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStart();
    void onStop();
    void onDeviceChanged(int index);
    void onTileCountChanged(int index);
    void onActiveTileChanged(int index);
    void onControlsChanged(const TileState& state);
    void onColorBarsToggled(bool checked);
    void onCaptureStatus();
    void onPumpFrames();
    void onPictureLineClicked(int line);

private:
    void applyTileVisibility();
    void refreshDeviceList();
    void syncLineMarkers();

    FrameQueue queue_;
    std::unique_ptr<DeckLinkCapture> capture_;

    QComboBox* deviceCombo_ = nullptr;
    QComboBox* tileCountCombo_ = nullptr;
    QComboBox* activeTileCombo_ = nullptr;
    QCheckBox* colorBarsCheck_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    TileControls* controls_ = nullptr;
    std::array<WfmGlWidget*, 4> tiles_{};
    std::array<TileState, 4> tileStates_{};
    int visibleTiles_ = 1;
    int activeTile_ = 0;
    QTimer* pumpTimer_ = nullptr;
};
