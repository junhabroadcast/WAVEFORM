#include "ui/TileControls.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>

TileControls::TileControls(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QFormLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    mode_ = new QComboBox(this);
    mode_->addItem(QStringLiteral("Waveform"), int(WfmDisplayMode::Waveform));
    mode_->addItem(QStringLiteral("Vector"), int(WfmDisplayMode::Vector));
    mode_->addItem(QStringLiteral("Lightning"), int(WfmDisplayMode::Lightning));
    layout->addRow(QStringLiteral("Mode"), mode_);

    style_ = new QComboBox(this);
    style_->addItem(QStringLiteral("Parade"), int(WaveformStyle::Parade));
    style_->addItem(QStringLiteral("Overlay"), int(WaveformStyle::Overlay));
    layout->addRow(QStringLiteral("Style"), style_);

    sweep_ = new QComboBox(this);
    sweep_->addItem(QStringLiteral("Line"), int(SweepMode::Line));
    sweep_->addItem(QStringLiteral("Field"), int(SweepMode::Field));
    layout->addRow(QStringLiteral("Sweep"), sweep_);

    gain_ = new QComboBox(this);
    gain_->addItem(QStringLiteral("1x"), 1.0);
    gain_->addItem(QStringLiteral("2x"), 2.0);
    gain_->addItem(QStringLiteral("5x"), 5.0);
    layout->addRow(QStringLiteral("Gain"), gain_);

    varEnable_ = new QCheckBox(QStringLiteral("Var Gain"), this);
    varGain_ = new QDoubleSpinBox(this);
    varGain_->setRange(0.1, 20.0);
    varGain_->setSingleStep(0.1);
    varGain_->setValue(1.0);
    layout->addRow(varEnable_, varGain_);

    mag_ = new QComboBox(this);
    mag_->addItem(QStringLiteral("1x"), 1.0);
    mag_->addItem(QStringLiteral("5x"), 5.0);
    mag_->addItem(QStringLiteral("10x"), 10.0);
    mag_->addItem(QStringLiteral("20x"), 20.0);
    layout->addRow(QStringLiteral("Mag"), mag_);

    lineSelect_ = new QCheckBox(QStringLiteral("Line Select"), this);
    line_ = new QSpinBox(this);
    line_->setRange(0, 4320);
    layout->addRow(lineSelect_, line_);

    freeze_ = new QCheckBox(QStringLiteral("Freeze"), this);
    bars75_ = new QCheckBox(QStringLiteral("75% Targets"), this);
    bars75_->setChecked(true);
    layout->addRow(freeze_);
    layout->addRow(bars75_);

    compY_ = new QCheckBox(QStringLiteral("Y"), this);
    compCb_ = new QCheckBox(QStringLiteral("Cb"), this);
    compCr_ = new QCheckBox(QStringLiteral("Cr"), this);
    compY_->setChecked(true);
    compCb_->setChecked(true);
    compCr_->setChecked(true);
    layout->addRow(QStringLiteral("Components"), compY_);
    layout->addRow(QString(), compCb_);
    layout->addRow(QString(), compCr_);

    persistence_ = new QSlider(Qt::Horizontal, this);
    persistence_->setRange(50, 99);
    persistence_->setValue(92);
    layout->addRow(QStringLiteral("Persistence"), persistence_);

    intensity_ = new QSlider(Qt::Horizontal, this);
    intensity_->setRange(5, 100);
    intensity_->setValue(35);
    layout->addRow(QStringLiteral("Intensity"), intensity_);

    connect(mode_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TileControls::emitState);
    connect(style_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TileControls::emitState);
    connect(sweep_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TileControls::emitState);
    connect(gain_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TileControls::emitState);
    connect(mag_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TileControls::emitState);
    connect(varEnable_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(varGain_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TileControls::emitState);
    connect(lineSelect_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(line_, QOverload<int>::of(&QSpinBox::valueChanged), this, &TileControls::emitState);
    connect(freeze_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(bars75_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(compY_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(compCb_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(compCr_, &QCheckBox::toggled, this, &TileControls::emitState);
    connect(persistence_, &QSlider::valueChanged, this, &TileControls::emitState);
    connect(intensity_, &QSlider::valueChanged, this, &TileControls::emitState);
}

void TileControls::setState(const TileState& state)
{
    const QSignalBlocker b1(mode_);
    mode_->setCurrentIndex(int(state.mode));
    style_->setCurrentIndex(int(state.style));
    sweep_->setCurrentIndex(int(state.sweep));
    const int gi = gain_->findData(double(state.gain));
    if (gi >= 0)
        gain_->setCurrentIndex(gi);
    varEnable_->setChecked(state.varGainEnabled);
    varGain_->setValue(state.varGain);
    const int mi = mag_->findData(double(state.mag));
    if (mi >= 0)
        mag_->setCurrentIndex(mi);
    lineSelect_->setChecked(state.lineSelectEnabled);
    line_->setValue(state.selectedLine);
    freeze_->setChecked(state.freeze);
    bars75_->setChecked(state.bars75);
    compY_->setChecked(hasFlag(state.components, ComponentFlags::Y));
    compCb_->setChecked(hasFlag(state.components, ComponentFlags::Cb));
    compCr_->setChecked(hasFlag(state.components, ComponentFlags::Cr));
    persistence_->setValue(int(state.persistence * 100.0f));
    intensity_->setValue(int(state.intensity * 100.0f));
}

TileState TileControls::state() const
{
    TileState s;
    s.mode = WfmDisplayMode(mode_->currentData().toInt());
    s.style = WaveformStyle(style_->currentData().toInt());
    s.sweep = SweepMode(sweep_->currentData().toInt());
    s.gain = float(gain_->currentData().toDouble());
    s.varGainEnabled = varEnable_->isChecked();
    s.varGain = float(varGain_->value());
    s.mag = float(mag_->currentData().toDouble());
    s.lineSelectEnabled = lineSelect_->isChecked();
    s.selectedLine = line_->value();
    s.freeze = freeze_->isChecked();
    s.bars75 = bars75_->isChecked();
    s.components = ComponentFlags::None;
    if (compY_->isChecked())
        s.components = s.components | ComponentFlags::Y;
    if (compCb_->isChecked())
        s.components = s.components | ComponentFlags::Cb;
    if (compCr_->isChecked())
        s.components = s.components | ComponentFlags::Cr;
    s.persistence = persistence_->value() / 100.0f;
    s.intensity = intensity_->value() / 100.0f;
    return s;
}

void TileControls::emitState()
{
    emit stateChanged(state());
}
