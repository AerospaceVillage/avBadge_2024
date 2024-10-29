#include "circularkeyboard.h"
#include "winglet-ui/theme.h"
#include "wingletgui.h"

#include <QLabel>
#include <QKeyEvent>
#include <cmath>

using namespace WingletUI;

const QList<QString> CircularKeyboard::fullKeyboard = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ :;'\"[]",
    "abcdefghijklmnopqrstuvwxyz ,.!?-/",
    "01234567890=`\\@#$%^&*(){}~|<>"
};

const QString singleKbdInstructions = "<html><head/><body><p>"
    "<span style=\"font-weight:bold\">Wheel:</span> Move Selection<br />"
    "<span style=\"font-weight:bold\">Wheel Click</span>: Put Char<br />"
    "<span style=\"font-weight:bold\">A Btn</span>: Submit<br />"
    "<span style=\"font-weight:bold\">B Btn</span>: Backspace/Exit<br />"
    "<span style=\"font-weight:bold\">Up/Down:</span> Move Selection"
    "</p></body></html>";

const QString multiKbdInstructions = "<html><head/><body><p>"
    "<span style=\"font-weight:bold\">Wheel:</span> Move Selection<br />"
    "<span style=\"font-weight:bold\">Wheel Click</span>: Put Char<br />"
    "<span style=\"font-weight:bold\">A Btn</span>: Submit<br />"
    "<span style=\"font-weight:bold\">B Btn</span>: Backspace/Exit<br />"
    "<span style=\"font-weight:bold\">Up/Down:</span> Move Selection<br />"
    "<span style=\"font-weight:bold\">Left/Right:</span> Change Keyboard"
    "</p></body></html>";

CircularKeyboard::CircularKeyboard(QList<QString> keyboardChars, QWidget* parent): QWidget{parent},
        keyboardChars(keyboardChars), standardFont(activeTheme->standardFont, 16),
        highlightedFont(activeTheme->standardFont, 24) {

    assert(keyboardChars.size() > 0);
    selectedKbdIdx = 0;

    // Draw Center Elements
    textBox = new QLabel(this);
    textBox->setAlignment(Qt::AlignCenter);
    textBox->setFont(standardFont);
    textBox->setForegroundRole(QPalette::WindowText);
    textBox->setMaximumWidth(380);
    refreshTextBox();

    titleBox = new QLabel(this);
    titleBox->setAlignment(Qt::AlignCenter);
    titleBox->setFont(QFont(activeTheme->titleFont, 16));
    titleBox->setForegroundRole(QPalette::Text);
    titleBox->setText("");
    titleBox->setFixedSize(280, 60);
    titleBox->setWordWrap(true);
    moveCenter(titleBox, 240, 150);

    instructionsBox = new QLabel(this);
    instructionsBox->setFont(QFont(activeTheme->standardFont, 10));
    instructionsBox->setForegroundRole(QPalette::HighlightedText);
    instructionsBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (keyboardChars.size() == 1)
        instructionsBox->setText(singleKbdInstructions);
    else
        instructionsBox->setText(multiKbdInstructions);
    instructionsBox->ensurePolished();
    instructionsBox->adjustSize();
    moveCenter(instructionsBox, 240, 340);

    redrawKeyboard();

    // Add logo
    avLogoLabel = new QLabel(this);
    activeTheme->renderBgAvLogo(avLogoLabel);
}

void CircularKeyboard::setValue(const QString& val) {
    if (m_maxLength >= 0 && val.length() > m_maxLength) {
        userInputString = val.left(m_maxLength);
    }
    else {
        userInputString = val;
    }
    refreshTextBox();
}

void CircularKeyboard::setMaxLength(int len)
{
    m_maxLength = len;
    if (len >= 0 && userInputString.length() > len) {
        userInputString = userInputString.left(len);
        refreshTextBox();
    }
}

void CircularKeyboard::setPrompt(const QString &prompt) {
    m_prompt = prompt + "\n";
    refreshTextBox();
}

void CircularKeyboard::wheelEvent(QWheelEvent *ev) {

    if (ev->angleDelta().y() > 0) {
        onEntrySelectionPrev();
    }
    else {
        onEntrySelectionNext();
    }
}

void CircularKeyboard::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        onSetKeyboardPrev();
        break;
    case Qt::Key_Right:
        onSetKeyboardNext();
        break;
    case Qt::Key_Up:
        onEntrySelectionPrev();
        break;
    case Qt::Key_Down:
        onEntrySelectionNext();
        break;
    case Qt::Key_Return:
        onEntrySelectionConfirm();
        break;
    case Qt::Key_A:
        if (userInputString.size() || m_allowEmptyInput) {
            int pos = 0;
            if (!m_validator || m_validator->validate(userInputString, pos) == QValidator::Acceptable) {
                m_entrySuccessful = true;
                emit entryComplete(userInputString);
                WingletGUI::inst->removeWidgetOnTop(this);
            }
            else {
                WingletGUI::inst->showMessageBox(m_validatorFailedMsg, "Invalid Input", "Back");
            }
        }
        break;
    case Qt::Key_B:
        onEntrySelectionBackspace();
        break;
    default:
        event->ignore();
        break;
    }
}

void CircularKeyboard::onSetKeyboardPrev() {
    if (keyboardChars.size() == 1)
        return;

    if (selectedKbdIdx == 0) {
        selectedKbdIdx = keyboardChars.size()-1;
    }
    else {
        selectedKbdIdx--;
    }

    double selectedAngle = keysAngleIncrement*selectedEntryIdx - keysStartAngle;
    redrawKeyboard(selectedAngle);
}

void CircularKeyboard::onSetKeyboardNext() {
    if (keyboardChars.size() == 1)
        return;

    if (selectedKbdIdx == keyboardChars.size()-1) {
        selectedKbdIdx = 0;
    }
    else {
        selectedKbdIdx++;
    }

    double selectedAngle = keysAngleIncrement*selectedEntryIdx - keysStartAngle;
    redrawKeyboard(selectedAngle);
}

void CircularKeyboard::onEntrySelectionPrev() {
    lastSelectedEntryIdx = selectedEntryIdx;
    if (selectedEntryIdx == 0) {
        selectedEntryIdx = character_widgets.size()-1;
    }
    else {
        selectedEntryIdx--;
    }

    refreshSelectedIndex();
}

void CircularKeyboard::onEntrySelectionNext() {
    lastSelectedEntryIdx = selectedEntryIdx;
    if (selectedEntryIdx == character_widgets.size()-1) {
        selectedEntryIdx = 0;
    }
    else {
        selectedEntryIdx++;
    }

    refreshSelectedIndex();
}

void CircularKeyboard::onEntrySelectionConfirm() {
    if (m_maxLength >= 0 && userInputString.length() >= m_maxLength)
        return;

    userInputString += keyboardChars.at(selectedKbdIdx).at(selectedEntryIdx);

    refreshTextBox();
}

void CircularKeyboard::onEntrySelectionBackspace() {
    if (userInputString.length() > 0) {
        userInputString = userInputString.left(userInputString.length() - 1);
        refreshTextBox();
    }
    else {
        // No more things to backspace, just go back to previous screen
        WingletGUI::inst->removeWidgetOnTop(this);
    }
}

void CircularKeyboard::fixIndexCenter(int idx)
{
    const int radius = 200;
    const int x_center = 240;
    const int y_center = 240;

    int x_pos = x_center+(radius*-cos(keysAngleIncrement*idx - keysStartAngle));
    int y_pos = y_center+(radius*sin(keysAngleIncrement*idx - keysStartAngle));
    moveCenter(character_widgets[idx], x_pos, y_pos);
}

void CircularKeyboard::redrawKeyboard(double targetAngle)
{
    for (QWidget* widget : character_widgets) {
        delete widget;
    }
    character_widgets.clear();

    // ComputredrawKeyboarde Layout
    QString selectedKbdChars = keyboardChars.at(selectedKbdIdx);
    int vec_size = selectedKbdChars.size();
    keysAngleIncrement = 2 * M_PI / vec_size;
    keysStartAngle = 0;
    if (keysAngleIncrement > 0.4) {
        keysAngleIncrement = 0.25;
        keysStartAngle = keysAngleIncrement * ((vec_size - 1) / 2.0);
    }

    // Draw Characters
    for (int i = 0; i < vec_size; i++) {
        QLabel* charLabel = new QLabel(this);
        character_widgets.append(charLabel);

        QChar val = selectedKbdChars.at(i);
        if (val == ' ') {
            charLabel->setText("'    '");
        }
        else {
            charLabel->setText(val);
        }
        charLabel->setAlignment(Qt::AlignCenter);
        charLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        charLabel->setFont(standardFont);
        charLabel->setForegroundRole(QPalette::WindowText);
        charLabel->setFixedSize(50, 50);
        charLabel->resize(50, 50);
        charLabel->ensurePolished();

        fixIndexCenter(i);
        charLabel->show();
    }

    int targetIdx = round((targetAngle + keysStartAngle) / keysAngleIncrement);
    if (targetIdx >= vec_size) {
        targetIdx = vec_size - 1;
    }
    else if (targetIdx < 0) {
        targetIdx = 0;
    }

    selectedEntryIdx = targetIdx;
    lastSelectedEntryIdx = -1;
    lastSelectedEntry = nullptr;

    refreshSelectedIndex();
}

void CircularKeyboard::refreshSelectedIndex()
{
    if (lastSelectedEntryIdx >= 0) {
        lastSelectedEntry = character_widgets.at(lastSelectedEntryIdx);
        lastSelectedEntry->setFont(standardFont);
        lastSelectedEntry->setForegroundRole(QPalette::WindowText);
    }
    selectedEntry = character_widgets.at(selectedEntryIdx);
    selectedEntry->setFont(highlightedFont);
    selectedEntry->setForegroundRole(QPalette::HighlightedText);
}

void CircularKeyboard::refreshTextBox()
{
    textBox->setText(m_prompt + userInputString);
    textBox->ensurePolished();
    textBox->adjustSize();
    moveCenter(textBox, 240, 240);
}
