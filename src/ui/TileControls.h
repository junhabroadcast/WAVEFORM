#pragma once

#include "display/DisplayModes.h"

#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QCheckBox;
class QSpinBox;
class QSlider;

class TileControls : public QWidget {
    Q_OBJECT
public:
    explicit TileControls(QWidget* parent = nullptr);

    void setState(const TileState& state);
    TileState state() const;

signals:
    void stateChanged(const TileState& state);

private:
    void emitState();

    QComboBox* mode_ = nullptr;
    QComboBox* style_ = nullptr;
    QComboBox* sweep_ = nullptr;
    QComboBox* gain_ = nullptr;
    QDoubleSpinBox* varGain_ = nullptr;
    QCheckBox* varEnable_ = nullptr;
    QComboBox* mag_ = nullptr;
    QCheckBox* lineSelect_ = nullptr;
    QSpinBox* line_ = nullptr;
    QCheckBox* freeze_ = nullptr;
    QCheckBox* bars75_ = nullptr;
    QCheckBox* compY_ = nullptr;
    QCheckBox* compCb_ = nullptr;
    QCheckBox* compCr_ = nullptr;
    QSlider* persistence_ = nullptr;
    QSlider* intensity_ = nullptr;
};
