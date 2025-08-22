#include "canardboard.h"
#include "wingletgui.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include "winglet-ui/hardware/canardinterface.h"
#include "winglet-ui/theme.h"
#include "winglet-ui/window/settingsmenu.h"

namespace WingletUI {

const QString canardInstructions = QStringLiteral("<html><head/><body><p>"
    "<span style=\"font-weight:bold\">Wheel:</span> Volume Selection<br />"
    "<span style=\"font-weight:bold\">Wheel Click</span>: Cycle Options<br />"
    "<span style=\"font-weight:bold\">Left/Right:</span> Move Digit<br />"
    "<span style=\"font-weight:bold\">Up/Down:</span> Change Digit<br />"
    "<span style=\"font-weight:bold\">A Btn</span>: Additional Settings<br />"
    "<span style=\"font-weight:bold\">B Btn</span>: Exit<br />"
    "</p></body></html>");

const QString qsliderStyle = QStringLiteral(
            "QSlider::groove:horizontal {\n"
            "    border: 1px solid #999999;\n"
            "    height: 8px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */\n"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);\n"
            "    margin: 2px 0;\n"
            "}\n"
            "\n"
            "QSlider::handle:horizontal {\n"
            "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);\n"
            "    border: 1px solid #5c5c5c;\n"
            "    width: 18px;\n"
            "    margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */\n"
            "    border-radius: 3px;\n"
            "}\n");


const QString QSpinBox_css_Normal      = QStringLiteral("QSpinBox { font-size: 56; padding: 2px; border-radius: 5px; border-width: 2px; border-style: solid; outline: none; border-color: #22d21fff; background-color: rgba(140, 140, 160, 0.77); }");
const QString QSpinBox_css_Highlighted = QStringLiteral("QSpinBox { font-size: 56; padding: 2px; border-radius: 5px; border-width: 2px; border-style: solid; border-color: #356a59; background-color: rgba(80, 140, 160, 1); }");
const QString QSpinBox_css_GrayedOut   = QStringLiteral("QSpinBox { font-size: 56; border-radius: 5px; background-color: #3b4b4773;  color: #9c545448; }");

CanardBoard::CanardBoard(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 40, 10, 10);
    mainLayout->setSpacing(20);

    savedFmFreq = WingletGUI::inst->settings.canardLastFmFreq();
    savedFmPreset = WingletGUI::inst->settings.canardLastFmPreset();
    savedAirbandFreq = WingletGUI::inst->settings.canardLastAirbandFreq();
    savedAirbandPreset = WingletGUI::inst->settings.canardLastAirbandPreset();

    QLabel *title = new QLabel("Tuner Remote", this);
    title->setFont(QFont(activeTheme->titleFont, 20));
    title->setForegroundRole(QPalette::Text);
    title->setAutoFillBackground(true);
    mainLayout->addWidget(title, 0, Qt::AlignHCenter);

    QHBoxLayout *spinboxRow = new QHBoxLayout();
    spinboxRow->setSpacing(6);

    for(int i=0; i<6; i++){
        if (i == DIGITS_BEFORE_DECIMAL) {
            decimalPointLabel = new QLabel(".", this);
            decimalPointLabel->setFont(QFont(activeTheme->standardFont, 30));
            decimalPointLabel->setAlignment(Qt::AlignCenter);
            decimalPointLabel->setFixedSize(10, 60);
            spinboxRow->addWidget(decimalPointLabel, 0);
        }

        QSpinBox *tmp = new QSpinBox(this);
        tmp->setFixedSize(35, 60);
        tmp->setRange(0, 9);
        spinboxRow->addWidget(tmp, 0, Qt::AlignCenter);

        tmp->setWrapping(true);
        tmp->setButtonSymbols(QAbstractSpinBox::NoButtons);
        tmp->setFont(QFont(activeTheme->standardFont, 24));
        tmp->setStyleSheet(QSpinBox_css_Normal);
        tmp->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        freqBoxes.append(tmp);
    }

    mainLayout->addLayout(spinboxRow);
    mainLayout->setAlignment(spinboxRow, Qt::AlignHCenter);

    QVBoxLayout *presetLayout = new QVBoxLayout();
    presetLayout->setSpacing(0);
    presetLabelLine1 = new QLabel(this);
    presetLabelLine1->setFont(QFont(activeTheme->standardFont, 20));
    presetLabelLine1->setAlignment(Qt::AlignCenter);
    presetLabelLine1->setFixedSize(300, 30);
    presetLabelLine1->setWordWrap(false);
    presetLayout->addWidget(presetLabelLine1, 0, Qt::AlignHCenter);

    presetLabelLine2 = new QLabel(this);
    presetLabelLine2->setFont(QFont(activeTheme->standardFont, 20));
    presetLabelLine2->setAlignment(Qt::AlignCenter);
    presetLabelLine2->setFixedSize(300, 30);
    presetLayout->addWidget(presetLabelLine2, 0, Qt::AlignHCenter);

    mainLayout->addLayout(presetLayout);
    // mainLayout->setAlignment(presetRow, Qt::AlignHCenter);

    //TO DO: set the original freq to what canard is already set to
    currentMode = WingletGUI::inst->saoMonitor->canard->getRadioState();

    presetModeLabel = new QLabel(this);
    presetModeLabel->setFont(QFont(activeTheme->standardFont, 16, QFont::Bold));
    presetModeLabel->setAlignment(Qt::AlignCenter);
    presetModeLabel->setFixedWidth(300);
    mainLayout->addWidget(presetModeLabel, 0, Qt::AlignHCenter);

    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setFixedWidth(220);
    volumeSlider->setStyleSheet(qsliderStyle);
    mainLayout->addWidget(volumeSlider, 0, Qt::AlignHCenter);
    
    radioModeLabel = new QLabel(this);
    radioModeLabel->setFont(QFont(activeTheme->standardFont, 16, QFont::Bold));
    radioModeLabel->setAlignment(Qt::AlignCenter);
    radioModeLabel->setFixedWidth(300);
    mainLayout->addWidget(radioModeLabel, 0, Qt::AlignHCenter);

    QLabel *instructionsLabel = new QLabel(canardInstructions, this);
    instructionsLabel->setFont(QFont(activeTheme->standardFont, 10));
    instructionsLabel->setForegroundRole(QPalette::HighlightedText);
    instructionsLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    instructionsLabel->setGeometry(150,280,300,200);
    instructionsLabel->setFont(QFont(activeTheme->standardFont));

    statusBar = new StatusBar(this);

    bgLogoLabel = new QLabel(this);
    activeTheme->loadMonochromeIcon(&bgLogo, ":/images/canard_logo.png", QPalette::Shadow);
    bgLogoLabel->resize(bgLogo.size());
    bgLogoLabel->setPixmap(bgLogo);
    bgLogoLabel->lower();
    moveCenter(bgLogoLabel, 240, 240);
    mainLayout->addStretch(1);

    renderTuningInfo();
    connect(WingletGUI::inst->saoMonitor->canard, SIGNAL(connectionStateChanged(bool)), this, SLOT(canardConnectionChanged(bool)));
}

CanardBoard::~CanardBoard(){
    // Main menu sets us in badge control mode before calling us
    WingletGUI::inst->saoMonitor->canard->setBadgeControlMode(false);

    // Save current item state to saved variables
    if (currentMode.presetMode) {
        if (currentMode.mode == CanardInterface::MODE_AIRBAND)
            savedAirbandPreset = currentMode.tuning.preset;
        else if (currentMode.mode == CanardInterface::MODE_FM)
            savedFmPreset = currentMode.tuning.preset;
    }
    else {
        if (currentMode.mode == CanardInterface::MODE_AIRBAND)
            savedAirbandFreq = currentMode.tuning.freq;
        else if (currentMode.mode == CanardInterface::MODE_FM)
            savedFmFreq = currentMode.tuning.freq;
    }

    // Commit saved variables to settings
    WingletGUI::inst->settings.setCanardLastFmFreq(savedFmFreq);
    WingletGUI::inst->settings.setCanardLastFmPreset(savedFmPreset);
    WingletGUI::inst->settings.setCanardLastAirbandFreq(savedAirbandFreq);
    WingletGUI::inst->settings.setCanardLastAirbandPreset(savedAirbandPreset);

    freqBoxes.clear();

    delete volumeSlider;
    delete statusBar;
    delete bgLogoLabel;
}

void CanardBoard::showEvent(QShowEvent *event) {
    (void) event;

    // Immediately remove this widget if we are coming back from settings after unplug
    if (!WingletGUI::inst->saoMonitor->canard->connected()) {
        WingletGUI::inst->removeWidgetOnTop(this);
        return;
    }

    // Re-render when window shown (in case settings updated something)
    renderTuningInfo();
}

void CanardBoard::renderVolumeValue() {
    volumeSlider->setValue(WingletGUI::inst->saoMonitor->canard->getVolume());
}

void CanardBoard::renderTuningInfo() {
    if (currentMode.presetMode) {
        decimalPointLabel->setVisible(false);
        for (int i = 0; i < freqBoxes.size(); i++) {
            freqBoxes[i]->setVisible(false);
            presetLabelLine2->setText(WingletGUI::inst->canardSettings->getAirbandPresetName(currentMode.tuning.preset));
        }

        presetLabelLine1->setVisible(true);
        presetLabelLine2->setVisible(true);
        if (widgetIndex == WIDGET_IDX_FREQ) {
            presetLabelLine1->setForegroundRole(QPalette::Link);
            presetLabelLine2->setForegroundRole(QPalette::Link);
        }
        else {
            presetLabelLine1->setForegroundRole(QPalette::WindowText);
            presetLabelLine2->setForegroundRole(QPalette::WindowText);
        }
        renderPresetInfo();
    }
    else {
        decimalPointLabel->setVisible(true);
        presetLabelLine1->setVisible(false);
        presetLabelLine2->setVisible(false);
        for (int i = 0; i < freqBoxes.size(); i++) {
            freqBoxes[i]->setVisible(true);
        }

        int numDecimal = currentMode.getDecimalCount();
        if (frequencyIndex >= DIGITS_BEFORE_DECIMAL + numDecimal) {
            frequencyIndex = DIGITS_BEFORE_DECIMAL + numDecimal - 1;
        }

        // Render all the boxes
        for (int i = 0; i < DIGITS_BEFORE_DECIMAL + numDecimal; i++) {
            if (i == frequencyIndex && widgetIndex == WIDGET_IDX_FREQ)
                freqBoxes[i]->setStyleSheet(QSpinBox_css_Highlighted);
            else
                freqBoxes[i]->setStyleSheet(QSpinBox_css_Normal);
        }

        // Gray out the remainder of boxes
        for (int i = DIGITS_BEFORE_DECIMAL + numDecimal; i < freqBoxes.size(); i++) {
            freqBoxes[i]->setStyleSheet(QSpinBox_css_GrayedOut);
        }

        renderFrequencyValue();
    }

    bool presetModeSelected = (widgetIndex == WIDGET_IDX_TUNE_PRESET_SEL);
    if (currentMode.presetMode)
        presetModeLabel->setText(presetModeSelected ? "< Preset Tuning" : "Preset Tuning");
    else
        presetModeLabel->setText(presetModeSelected ? "Direct Tuning >" : "Direct Tuning");
    presetModeLabel->setForegroundRole(presetModeSelected ? QPalette::Link : QPalette::HighlightedText);

    bool modeSelected = (widgetIndex == WIDGET_IDX_RADIO_MODE);
    switch (currentMode.mode) {
    case CanardInterface::MODE_AIRBAND:
        radioModeLabel->setText(modeSelected ? "Airband Mode >" : "Airband Mode");
        break;
    case CanardInterface::MODE_FM:
        radioModeLabel->setText(modeSelected ? "< FM Mode" : "FM Mode");
        break;
    }
    radioModeLabel->setForegroundRole(modeSelected ? QPalette::Link : QPalette::HighlightedText);

    renderVolumeValue();
}

void CanardBoard::renderFrequencyValue() {
    if (currentMode.presetMode)
        return;

    uint32_t frequency = currentMode.tuning.freq;
    int numDecimal = currentMode.getDecimalCount();

    int scaleFactor = 1;
    for (int i = 0; i < numDecimal; i++) {
        scaleFactor *= 10;
    }
    uint32_t intComponent = frequency / scaleFactor;
    freqBoxes[0]->setValue(intComponent / 100);
    freqBoxes[1]->setValue((intComponent / 10) % 10);
    freqBoxes[2]->setValue(intComponent % 10);

    uint32_t fracComponent = frequency % scaleFactor;
    for (int i = numDecimal - 1; i >= 0; i--) {
        if (i < 3) {
            freqBoxes[3 + i]->setValue(fracComponent % 10);
        }
        fracComponent /= 10;
    }
}

void CanardBoard::renderPresetInfo() {
    QString presetName, presetFreq;
    switch (currentMode.mode) {
    case CanardInterface::MODE_AIRBAND:
        presetName = WingletGUI::inst->canardSettings->getAirbandPresetName(currentMode.tuning.preset);
        presetFreq = WingletGUI::inst->canardSettings->getAirbandPresetFreq(currentMode.tuning.preset);
        break;
    case CanardInterface::MODE_FM:
        presetName = WingletGUI::inst->canardSettings->getFmPresetName(currentMode.tuning.preset);
        presetFreq = WingletGUI::inst->canardSettings->getFmPresetFreq(currentMode.tuning.preset);
        break;
    }

    if (presetName.isEmpty()) {
        presetLabelLine1->setText(QString("Preset %1").arg(currentMode.tuning.preset));
    } else {
        presetLabelLine1->setText(QString("#%1: %2").arg(currentMode.tuning.preset).arg(presetName));
    }
    presetLabelLine2->setText(presetFreq);
}

uint CanardBoard::getFrequencyTuningIncrement() {
    if (currentMode.presetMode)
        return 0;

    int multiplyLevel = (DIGITS_BEFORE_DECIMAL + currentMode.getDecimalCount() - 1) - frequencyIndex;
    if (multiplyLevel < 0)
        return 0;
    int increment = 1;
    for (int i = 0; i < multiplyLevel; i++) {
        increment *= 10;
    }
    return increment;
}

void CanardBoard::increasePreset() {
    if (!currentMode.presetMode)
        return;

    uint maxPresets;
    switch (currentMode.mode) {
    case CanardInterface::MODE_AIRBAND:
        maxPresets = CanardSettings::AIRBAND_PRESET_COUNT;
        break;
    case CanardInterface::MODE_FM:
        maxPresets = CanardSettings::FM_PRESET_COUNT;
        break;
    default:
        return;
    }

    if (currentMode.tuning.preset < maxPresets) {
        currentMode.tuning.preset++;
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderPresetInfo();
    }    
}

void CanardBoard::decreasePreset() {
    if (!currentMode.presetMode)
        return;

    if (currentMode.tuning.preset > 1) {
        currentMode.tuning.preset--;
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderPresetInfo();
    }
}

void CanardBoard::increaseFrequency() {
    if (currentMode.presetMode)
        return;

    uint32_t newFreq;
    uint32_t tuningIncrement = getFrequencyTuningIncrement();
    // Manual override so last digit of airband is aligned to the 5 point
    if (frequencyIndex == 5 && tuningIncrement == 1) {
        newFreq = ((currentMode.tuning.freq / 5) + 1) * 5;
    }
    else {
        newFreq = currentMode.tuning.freq + tuningIncrement;
    }

    if (newFreq <= currentMode.getMaxFreq()) {
        currentMode.tuning.freq = newFreq;
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderFrequencyValue();
    }
    // Bound to valid frequency range if current out of range
    else if (currentMode.tuning.freq > currentMode.getMaxFreq()) {
        currentMode.tuning.freq = currentMode.getMaxFreq();
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderFrequencyValue();
    }
    else if (currentMode.tuning.freq < currentMode.getMinFreq()) {
        currentMode.tuning.freq = currentMode.getMinFreq();
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderFrequencyValue();
    }
}

void CanardBoard::decreaseFrequency() {
    if (currentMode.presetMode)
        return;

    uint32_t newFreq;
    uint32_t tuningIncrement = getFrequencyTuningIncrement();
    // Manual override so last digit of airband is aligned to the 5 point
    if (frequencyIndex == 5 && tuningIncrement == 1) {
        newFreq = ((currentMode.tuning.freq / 5) - 1) * 5;
    }
    else {
        newFreq = currentMode.tuning.freq - tuningIncrement;
    }

    if (currentMode.tuning.freq >= tuningIncrement && newFreq >= currentMode.getMinFreq()) {
        currentMode.tuning.freq = newFreq;
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderFrequencyValue();
    }
    // Bound to valid frequency range if current out of range
    else if (currentMode.tuning.freq > currentMode.getMaxFreq()) {
        currentMode.tuning.freq = currentMode.getMaxFreq();
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderFrequencyValue();
    }
    else if (currentMode.tuning.freq < currentMode.getMinFreq()) {
        currentMode.tuning.freq = currentMode.getMinFreq();
        WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
        renderFrequencyValue();
    }
}

void CanardBoard::nextFreqBox() {
    if (currentMode.presetMode)
        return;

    if((uint) frequencyIndex == DIGITS_BEFORE_DECIMAL + currentMode.getDecimalCount() - 1){
        frequencyIndex = 0;
    }
    else {
        frequencyIndex += 1;
    }
    renderTuningInfo();
}

void CanardBoard::prevFreqBox() {
    if (currentMode.presetMode)
        return;

    if (frequencyIndex == 0){
        frequencyIndex = DIGITS_BEFORE_DECIMAL + currentMode.getDecimalCount() - 1;
    }
    else {
        frequencyIndex -= 1;
    }
    renderTuningInfo();
}

void CanardBoard::keyPressEvent(QKeyEvent *ev) {
    switch ( ev->key() ) {
    case Qt::Key_Up:
        switch (widgetIndex) {
        case WIDGET_IDX_FREQ:
            if (currentMode.presetMode) {
                increasePreset();
            }
            else {
                increaseFrequency();
            }
            break;
        }
        break;
    case Qt::Key_Down:
        switch (widgetIndex) {
        case WIDGET_IDX_FREQ:
            if (currentMode.presetMode) {
                decreasePreset();
            }
            else {
                decreaseFrequency();
            }
            break;
        }
        break;
    case Qt::Key_Left:
        switch (widgetIndex) {
        case WIDGET_IDX_FREQ:
            if (currentMode.presetMode) {
                decreasePreset();
            }
            else {
                prevFreqBox();
            }
            break;
        case WIDGET_IDX_TUNE_PRESET_SEL:
            setTuneMode();
            break;
        case WIDGET_IDX_RADIO_MODE:
            setAirbandMode();
            break;
        }
        break;
    case Qt::Key_Right:
        switch (widgetIndex) {
        case WIDGET_IDX_FREQ:
            if (currentMode.presetMode) {
                increasePreset();
            }
            else {
                nextFreqBox();
            }
            break;
        case WIDGET_IDX_TUNE_PRESET_SEL:
            setPresetMode();
            break;
        case WIDGET_IDX_RADIO_MODE:
            setFMMode();
            break;
        }
        break;
    case Qt::Key_Return:
        if (widgetIndex + 1 == WIDGET_IDX_COUNT) {
            widgetIndex = 0;
        }
        else {
            widgetIndex++;
        }
        renderTuningInfo();
        break;
    case Qt::Key_A:
        // Goto the sub menu for canard that Robert made
        WingletGUI::inst->addWidgetOnTop(new SettingsMenu(WingletGUI::inst, true));
        break;

    default:
        ev->ignore();   //Ignore and let other widgets process (ie "B" back button)
    };
}

void CanardBoard:: wheelEvent(QWheelEvent *event)  {
    QPoint numDegrees = event->angleDelta();

    if (!numDegrees.isNull()) {
        changeVolume(numDegrees.y() / 30);
    }
}

void CanardBoard::setFMMode() {
    if (currentMode.mode == CanardInterface::MODE_FM)
        return;

    if (currentMode.presetMode) {
        // Save old frequency
        if (currentMode.mode == CanardInterface::MODE_AIRBAND)
            savedAirbandPreset = currentMode.tuning.preset;
    }
    else {
        // Save old frequency
        if (currentMode.mode == CanardInterface::MODE_AIRBAND)
            savedAirbandFreq = currentMode.tuning.freq;
    }

    currentMode.mode = CanardInterface::MODE_FM;
    if (currentMode.presetMode) {
        currentMode.tuning.preset = savedFmPreset;
    }
    else {
        currentMode.tuning.freq = savedFmFreq;
    }
    WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
    renderTuningInfo();
}

void CanardBoard::setAirbandMode() {
    if (currentMode.mode == CanardInterface::MODE_AIRBAND)
        return;

    // Save old value
    if (currentMode.presetMode) {
        if (currentMode.mode == CanardInterface::MODE_FM)
            savedFmPreset = currentMode.tuning.preset;
    }
    else {
        // Save old frequency
        if (currentMode.mode == CanardInterface::MODE_FM)
            savedFmFreq = currentMode.tuning.freq;
    }

    currentMode.mode = CanardInterface::MODE_AIRBAND;
    if (currentMode.presetMode) {
        currentMode.tuning.preset = savedAirbandPreset;
    }
    else {
        currentMode.tuning.freq = savedAirbandFreq;
    }
    WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
    renderTuningInfo();
}

void CanardBoard::setPresetMode() {
    if (currentMode.presetMode)
        return;

    // Save old value
    if (currentMode.mode == CanardInterface::MODE_FM)
        savedFmFreq = currentMode.tuning.freq;
    else if (currentMode.mode == CanardInterface::MODE_AIRBAND)
        savedAirbandFreq = currentMode.tuning.freq;

    currentMode.presetMode = true;

    // Swap over to preset mode
    if (currentMode.mode == CanardInterface::MODE_FM)
        currentMode.tuning.preset = savedFmPreset;
    else if (currentMode.mode == CanardInterface::MODE_AIRBAND)
        currentMode.tuning.preset = savedAirbandPreset;

    WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
    renderTuningInfo();
}

void CanardBoard::setTuneMode() {
    if (!currentMode.presetMode)
        return;

    // Save old value
    if (currentMode.mode == CanardInterface::MODE_AIRBAND)
        savedAirbandPreset = currentMode.tuning.preset;
    else if (currentMode.mode == CanardInterface::MODE_FM)
        savedFmPreset = currentMode.tuning.preset;

    currentMode.presetMode = false;

    // Swap over to preset mode
    if (currentMode.mode == CanardInterface::MODE_FM)
        currentMode.tuning.freq = savedFmFreq;
    else if (currentMode.mode == CanardInterface::MODE_AIRBAND)
        currentMode.tuning.freq = savedAirbandFreq;

    WingletGUI::inst->saoMonitor->canard->setRadioState(currentMode);
    renderTuningInfo();
}

void CanardBoard::changeVolume(int delta) {
    int oldValue = volumeSlider->value();
    volumeSlider->setValue(volumeSlider->value() + delta);
    if (oldValue != volumeSlider->value()) {
        WingletGUI::inst->saoMonitor->canard->setVolume(volumeSlider->value());
    }
}

void CanardBoard::canardConnectionChanged(bool connected) {
    if (!connected && isVisible()) {
        WingletGUI::inst->removeWidgetOnTop(this);
    }
}

} // namespace WingletUI
