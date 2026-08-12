#include "ui/MainWindow.h"
#include "render/WfmGlWidget.h"
#include "ui/TileControls.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("WAVEFORM — DeckLink Waveform / Vector / Lightning / Video"));
    resize(1400, 900);

    capture_ = std::make_unique<DeckLinkCapture>(queue_);
    connect(capture_.get(), &DeckLinkCapture::statusChanged, this, &MainWindow::onCaptureStatus);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);

    auto* left = new QVBoxLayout();
    root->addLayout(left, 1);

    auto* topBar = new QHBoxLayout();
    deviceCombo_ = new QComboBox(this);
    topBar->addWidget(new QLabel(QStringLiteral("Device:"), this));
    topBar->addWidget(deviceCombo_, 1);

    auto* startBtn = new QPushButton(QStringLiteral("Start"), this);
    auto* stopBtn = new QPushButton(QStringLiteral("Stop"), this);
    topBar->addWidget(startBtn);
    topBar->addWidget(stopBtn);

    colorBarsCheck_ = new QCheckBox(QStringLiteral("Color Bars"), this);
    colorBarsCheck_->setToolTip(QStringLiteral("Force internal 75% color bars instead of DeckLink SDI"));
    topBar->addWidget(colorBarsCheck_);

    tileCountCombo_ = new QComboBox(this);
    tileCountCombo_->addItem(QStringLiteral("1 Tile"), 1);
    tileCountCombo_->addItem(QStringLiteral("2 Tiles"), 2);
    tileCountCombo_->addItem(QStringLiteral("4 Tiles"), 4);
    topBar->addWidget(tileCountCombo_);

    activeTileCombo_ = new QComboBox(this);
    activeTileCombo_->addItem(QStringLiteral("Tile 1"), 0);
    activeTileCombo_->addItem(QStringLiteral("Tile 2"), 1);
    activeTileCombo_->addItem(QStringLiteral("Tile 3"), 2);
    activeTileCombo_->addItem(QStringLiteral("Tile 4"), 3);
    topBar->addWidget(activeTileCombo_);

    left->addLayout(topBar);

    auto* grid = new QGridLayout();
    for (int i = 0; i < 4; ++i) {
        tiles_[i] = new WfmGlWidget(this);
        tileStates_[i] = TileState{};
        if (i == 0)
            tileStates_[i].mode = WfmDisplayMode::Waveform;
        if (i == 1)
            tileStates_[i].mode = WfmDisplayMode::Vector;
        if (i == 2)
            tileStates_[i].mode = WfmDisplayMode::Lightning;
        if (i == 3)
            tileStates_[i].mode = WfmDisplayMode::Video;
        tiles_[i]->setTileState(tileStates_[i]);
        connect(tiles_[i], &WfmGlWidget::lineClicked, this, &MainWindow::onPictureLineClicked);
        grid->addWidget(tiles_[i], i / 2, i % 2);
    }
    left->addLayout(grid, 1);

    statusLabel_ = new QLabel(QStringLiteral("Stopped"), this);
    statusBar()->addWidget(statusLabel_, 1);

    controls_ = new TileControls(this);
    controls_->setMaximumWidth(280);
    controls_->setState(tileStates_[0]);
    root->addWidget(controls_);

    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(colorBarsCheck_, &QCheckBox::toggled, this, &MainWindow::onColorBarsToggled);
    connect(deviceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onDeviceChanged);
    connect(tileCountCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTileCountChanged);
    connect(activeTileCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onActiveTileChanged);
    connect(controls_, &TileControls::stateChanged, this, &MainWindow::onControlsChanged);

    pumpTimer_ = new QTimer(this);
    connect(pumpTimer_, &QTimer::timeout, this, &MainWindow::onPumpFrames);
    pumpTimer_->start(16);

    // Default to classic 4-tile QC layout
    tileCountCombo_->setCurrentIndex(2); // 4 Tiles
    visibleTiles_ = 4;

    refreshDeviceList();
    applyTileVisibility();
}

MainWindow::~MainWindow()
{
    if (capture_)
        capture_->stop();
}

void MainWindow::refreshDeviceList()
{
    deviceCombo_->blockSignals(true);
    deviceCombo_->clear();
    const QStringList devices = capture_->listDevices();
    if (devices.isEmpty()) {
        deviceCombo_->addItem(QStringLiteral("(No DeckLink — simulator)"), -1);
    } else {
        for (int i = 0; i < devices.size(); ++i)
            deviceCombo_->addItem(devices[i], i);
    }
    deviceCombo_->blockSignals(false);
}

void MainWindow::onStart()
{
    int idx = deviceCombo_->currentData().toInt();
    if (idx < 0)
        idx = 0;
    capture_->setSimulatorFallback(true);
    capture_->setForceColorBars(colorBarsCheck_->isChecked());
    capture_->start(idx);
    onCaptureStatus();
}

void MainWindow::onStop()
{
    capture_->stop();
    onCaptureStatus();
}

void MainWindow::onColorBarsToggled(bool checked)
{
    capture_->setForceColorBars(checked);
    deviceCombo_->setEnabled(!checked);
    onCaptureStatus();
}

void MainWindow::onDeviceChanged(int)
{
    // Restart on next Start click.
}

void MainWindow::onTileCountChanged(int)
{
    visibleTiles_ = tileCountCombo_->currentData().toInt();
    applyTileVisibility();
    syncLineMarkers();
}

void MainWindow::onActiveTileChanged(int)
{
    activeTile_ = activeTileCombo_->currentData().toInt();
    controls_->setState(tileStates_[activeTile_]);
}

void MainWindow::onControlsChanged(const TileState& state)
{
    tileStates_[activeTile_] = state;
    tiles_[activeTile_]->setTileState(state);
    syncLineMarkers();
}

void MainWindow::onPictureLineClicked(int line)
{
    // Clicking the picture selects that line on the scope tiles.
    // If no scope tile has Line Select on yet, turn it on for all of them.
    bool anyEnabled = false;
    for (int i = 0; i < visibleTiles_; ++i) {
        const TileState& s = tileStates_[i];
        if (s.mode != WfmDisplayMode::Video && s.mode != WfmDisplayMode::None && s.lineSelectEnabled)
            anyEnabled = true;
    }

    for (int i = 0; i < visibleTiles_; ++i) {
        TileState& s = tileStates_[i];
        if (s.mode == WfmDisplayMode::Video || s.mode == WfmDisplayMode::None)
            continue;
        if (!anyEnabled)
            s.lineSelectEnabled = true;
        if (s.lineSelectEnabled) {
            s.selectedLine = line;
            tiles_[i]->setTileState(s);
        }
    }

    controls_->setState(tileStates_[activeTile_]);
    syncLineMarkers();
}

void MainWindow::syncLineMarkers()
{
    // Line picked on any visible scope tile is echoed as a marker on Video tiles.
    int line = -1;
    for (int i = 0; i < visibleTiles_; ++i) {
        const TileState& s = tileStates_[i];
        if (s.mode != WfmDisplayMode::Video && s.mode != WfmDisplayMode::None && s.lineSelectEnabled) {
            line = s.selectedLine;
            break;
        }
    }
    for (int i = 0; i < 4; ++i)
        tiles_[i]->setMarkerLine(line);
}

void MainWindow::onCaptureStatus()
{
    statusLabel_->setText(QStringLiteral("%1 | pushed %2 | queue-drop %3 | captured %4")
                              .arg(capture_->statusText())
                              .arg(queue_.pushed())
                              .arg(queue_.dropped())
                              .arg(capture_->framesCaptured()));
}

void MainWindow::onPumpFrames()
{
    auto frame = queue_.takeLatest();
    if (!frame)
        return;

    const QString mode = capture_->modeName();
    const bool locked = capture_->signalLocked();
    const uint64_t drops = queue_.dropped();

    for (int i = 0; i < 4; ++i) {
        if (!tiles_[i]->isVisible())
            continue;
        tiles_[i]->setStatus(mode, locked, drops);
        tiles_[i]->setFrame(frame);
    }
    onCaptureStatus();
}

void MainWindow::applyTileVisibility()
{
    for (int i = 0; i < 4; ++i)
        tiles_[i]->setVisible(i < visibleTiles_);
}
