#ifndef CANARDBOARD_H
#define CANARDBOARD_H

#include <QWidget>
#include <QSpinBox>
#include <QList>
#include <QSlider>
#include "winglet-ui/hardware/canardinterface.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

class CanardBoard : public QWidget
{
    Q_OBJECT
public:
    explicit CanardBoard(QWidget *parent = nullptr);
    ~CanardBoard();
    void keyPressEvent(QKeyEvent *ev) override;
    void wheelEvent(QWheelEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void renderTuningInfo();
    void renderPresetInfo();
    void renderFrequencyValue();
    void renderVolumeValue();
    void increasePreset();
    void decreasePreset();
    void increaseFrequency();
    void decreaseFrequency();
    void nextFreqBox();
    void prevFreqBox();
    void changeVolume(int delta);
    uint getFrequencyTuningIncrement();
    void setFMMode();
    void setAirbandMode();
    void setPresetMode();
    void setTuneMode();

    CanardInterface::RadioState currentMode;
    int frequencyIndex = 0;
    int widgetIndex = 0;
    uint32_t savedFmFreq;
    uint32_t savedAirbandFreq;
    uint savedFmPreset;
    uint savedAirbandPreset;

    enum WidgetIndex {
        WIDGET_IDX_FREQ = 0,
        WIDGET_IDX_TUNE_PRESET_SEL,
        WIDGET_IDX_RADIO_MODE,
        WIDGET_IDX_COUNT,
    };

    QLabel* presetLabelLine1;
    QLabel* presetLabelLine2;
    QLabel* presetModeLabel;
    QLabel* radioModeLabel;

    QString keyboardChars;
    QLabel* decimalPointLabel;
    QList<QSpinBox*> freqBoxes ;
    QSlider* volumeSlider;
    StatusBar* statusBar;
    QLabel *bgLogoLabel;
    QPixmap bgLogo;

    const int DIGITS_BEFORE_DECIMAL = 3;

private slots:
    void canardConnectionChanged(bool connected);
};

} // namespace WingletUI

#endif // CANARDBOARD_H
