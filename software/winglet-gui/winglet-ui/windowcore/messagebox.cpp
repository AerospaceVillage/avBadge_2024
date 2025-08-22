#include "winglet-ui/theme.h"
#include "messagebox.h"
#include "wingletgui.h"
#include <QStringList>
#include <QKeyEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace WingletUI {

MessageBox::MessageBox(QWidget *parent, Qt::Alignment alignment)
    : QWidget{parent}, animationState(ANIMATE_HIDDEN),
      message(new ElidedLabel(this)), title(new ElidedLabel(this)),
      selectionPtr(new QLabel(this)), animationGroup(new QParallelAnimationGroup(this))
{
    QGraphicsOpacityEffect* effect;

    title->setGeometry(80, 62, 322, 75);
    title->setForegroundRole(QPalette::Text);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont(activeTheme->titleFont, 26));
    title->setMultiline(true);
    effect = new QGraphicsOpacityEffect(title);
    effect->setOpacity(0);
    title->setGraphicsEffect(effect);

    message->setGeometry(36, 140, 408, 200);
    message->setForegroundRole(QPalette::WindowText);
    message->setAlignment(alignment);
    message->setText("No Message");
    message->setFont(QFont(activeTheme->standardFont, 14));
    message->setMultiline(true);
    effect = new QGraphicsOpacityEffect(message);
    effect->setOpacity(0);
    message->setGraphicsEffect(effect);

    // Create the buttons
    for (int i = 0; i < MAX_BUTTONS; i++) {
        ElidedLabel* button = new ElidedLabel(this);
        button->resize(200, 30);
        if (i == 0)
            button->setText("Okay");
        else
            button->setText("Unnamed");
        button->setForegroundRole(QPalette::HighlightedText);
        button->setAlignment(Qt::AlignCenter);
        button->setFont(QFont(activeTheme->standardFont, 18));
        effect = new QGraphicsOpacityEffect(button);
        effect->setOpacity(0);
        button->setGraphicsEffect(effect);
        buttons[i] = button;
    }

    // Create selection ptr
    selectionPtr->setText(">");
    selectionPtr->setForegroundRole(QPalette::HighlightedText);
    selectionPtr->setAlignment(Qt::AlignCenter);
    selectionPtr->setFont(QFont(activeTheme->titleFont, 18, QFont::Bold));
    selectionPtr->setFixedSize(selectionPtr->sizeHint());
    effect = new QGraphicsOpacityEffect(selectionPtr);
    effect->setOpacity(0);
    selectionPtr->setGraphicsEffect(effect);

    // Rerender the buttons
    numVisibleButtons = 1;
    selectedIndex = 0;
    rerenderButtonLabels();

    connect(animationGroup, SIGNAL(finished()), this, SLOT(animationGroupFinished()));
}

MessageBox::~MessageBox()
{
    delete animationGroup;
    delete message;
    delete title;
    delete selectionPtr;
    for (int i = 0; i < MAX_BUTTONS; i++) {
        delete buttons[i];
    }
}

void MessageBox::setSingleButtonWithText(const QString &text) {
     numVisibleButtons = 1;
     selectedIndex = 0;
     buttons[0]->setText(text);
     rerenderButtonLabels();
}

void MessageBox::setButtons(const QStringList &btnList, int newIdx) {
    if (btnList.size() == 0) {
        qWarning("MessageBox::setButtons: Cannot set button to empty string list");
        return;
    }
    int newNumButtons = btnList.size();
    if (newNumButtons > MAX_BUTTONS) {
        qWarning("MessageBox::setButtons: Truncating button list (was %d) to max size %d", newNumButtons, MAX_BUTTONS);
        newNumButtons = MAX_BUTTONS;
    }
    for (int i = 0; i < newNumButtons; i++) {
        buttons[i]->setText(btnList[i]);
    }
    numVisibleButtons = newNumButtons;
    if (newIdx >= 0 && newIdx < numVisibleButtons) {
        selectedIndex = newIdx;
    }
    else {
        if (newIdx >= numVisibleButtons) {
            qWarning("MessageBox::setButtons: Provided newIdx (%d) larger than number of buttons (%d)", newIdx, numVisibleButtons);
        }
        // else, it's a negative number, and we're told to just keep the old index
        if (selectedIndex >= numVisibleButtons)
            selectedIndex = numVisibleButtons - 1;
    }

    rerenderButtonLabels();
}

void MessageBox::showEvent(QShowEvent *event)
{
    (void) event;
    animateShow();
}

void MessageBox::animationGroupFinished() {
    if (animationState == ANIMATE_SHOWING) {
        animationState = ANIMATE_SHOWN;
        rerenderCurrentSelection();
    }
    else if (animationState == ANIMATE_HIDING) {
        animationState = ANIMATE_HIDDEN;
        WingletGUI::inst->removeWidgetOnTop(this);
    }
    else if (animationState == ANIMATE_SELECTION) {
        animationState = ANIMATE_SHOWN;
        rerenderCurrentSelection();
    }
}

void MessageBox::addOpacityAnimation(QWidget *widget, float start, float end)
{
    QPropertyAnimation* anim = new QPropertyAnimation(widget->graphicsEffect(), "opacity");
    anim->setStartValue(start);
    anim->setDuration(activeTheme->animationDuration);
    anim->setEndValue(end);
    animationGroup->addAnimation(anim);
}

void MessageBox::animateShow()
{
    if (animationState != ANIMATE_HIDDEN)
        return;

    animationGroup->clear();
    addOpacityAnimation(message, 0.0, 1.0);
    addOpacityAnimation(title, 0.0, 1.0);
    for (int i = 0; i < numVisibleButtons; i++)
        addOpacityAnimation(buttons[i], 0.0, 1.0);
    addOpacityAnimation(selectionPtr, 0.0, 1.0);

    animationState = ANIMATE_SHOWING;
    animationGroup->start();
}

void MessageBox::animateHide()
{
    if (animationState != ANIMATE_SHOWN) {
        if (animationState == ANIMATE_HIDDEN)
            WingletGUI::inst->removeWidgetOnTop(this);
        return;
    }

    animationGroup->clear();
    addOpacityAnimation(message, 1.0, 0.0);
    addOpacityAnimation(title, 1.0, 0.0);
    for (int i = 0; i < numVisibleButtons; i++)
        addOpacityAnimation(buttons[i], 1.0, 0.0);
    addOpacityAnimation(selectionPtr, 1.0, 0.0);

    animationState = ANIMATE_HIDING;
    animationGroup->start();
}

void MessageBox::animateSelection(int startIdx, int endIdx) {
    if (animationState != ANIMATE_SHOWN) {
        return;
    }

    animationGroup->clear();
    QPropertyAnimation* anim = new QPropertyAnimation(selectionPtr, "pos");
    anim->setStartValue(calcSelectionPtrPosition(startIdx));
    anim->setEasingCurve(QEasingCurve::Type::OutQuart);
    anim->setDuration(activeTheme->animationDuration);
    anim->setEndValue(calcSelectionPtrPosition(endIdx));
    animationGroup->addAnimation(anim);
    animationState = ANIMATE_SELECTION;
    animationGroup->start();
}

void MessageBox::setSelectedIndex(int btnIdx) {
    if (btnIdx < 0 || btnIdx >= numVisibleButtons) {
        qWarning("MesageBox::setSelectedIndex: Attempting to set index to invalid value %d", btnIdx);
        return;
    }
    if (selectedIndex == btnIdx)
        return;


    if (animationState == ANIMATE_SELECTION) {
        qWarning("MessageBox::setSelectedIndex: Setting index while animation in progress. This will result in UI glitches");
    }
    selectedIndex = btnIdx;
    rerenderCurrentSelection();
}

void MessageBox::moveSelectionUp() {
    if (animationState != ANIMATE_SHOWN) {
        return;
    }

    if (numVisibleButtons < 2)
        return;

    if (selectedIndex < 1)
        return;

    int oldIdx = selectedIndex;
    QFont buttonFont = buttons[selectedIndex]->font();
    buttonFont.setUnderline(false);
    buttons[oldIdx]->setFont(buttonFont);

    selectedIndex--;
    animateSelection(oldIdx, selectedIndex);
}

void MessageBox::moveSelectionDown() {
    if (animationState != ANIMATE_SHOWN) {
        return;
    }

    if (numVisibleButtons < 2)
        return;

    if (selectedIndex + 1 >= numVisibleButtons)
        return;

    int oldIdx = selectedIndex;
    QFont buttonFont = buttons[selectedIndex]->font();
    buttonFont.setUnderline(false);
    buttons[oldIdx]->setFont(buttonFont);

    selectedIndex++;
    animateSelection(oldIdx, selectedIndex);
}

void MessageBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key::Key_A:
    case Qt::Key::Key_Return:
        if (animationState == ANIMATE_SHOWN) {
            // Show selection
            buttons[selectedIndex]->setForegroundRole(QPalette::Link);
            selectionPtr->setForegroundRole(QPalette::Link);

            // Emit the selection
            emit buttonClicked(selectedIndex);

            // Begin hiding
            animateHide();
        }
        break;
    case Qt::Key::Key_Up:
        moveSelectionUp();
        break;
    case Qt::Key::Key_Down:
        moveSelectionDown();
        break;
    case Qt::Key::Key_B:
        // Supress B keys (so the user must respond to the dialog)
        break;
    default:
        event->ignore();
    }
}

void MessageBox::wheelEvent(QWheelEvent *ev) {
    if (ev->angleDelta().y() < 0) {
        moveSelectionDown();
    }
    else {
        moveSelectionUp();
    }
    ev->accept();
}

void MessageBox::rerenderButtonLabels()
{
    switch (numVisibleButtons) {
    case 1:
        moveCenter(buttons[0], 240, 400);
        buttons[0]->show();
        buttons[1]->hide();
        buttons[2]->show();
        break;
    case 2:
        moveCenter(buttons[0], 240, 382);
        buttons[0]->show();
        moveCenter(buttons[1], 240, 417);
        buttons[1]->show();
        buttons[2]->hide();
        break;
    case 3:
        moveCenter(buttons[0], 240, 365);
        buttons[0]->show();
        moveCenter(buttons[1], 240, 400);
        buttons[1]->show();
        moveCenter(buttons[2], 240, 435);
        buttons[2]->show();
        break;
    default:
        qWarning("MessageBox::rerenderButtonLabels: Invalid number of visible buttons: %d", numVisibleButtons);
        break;
    }

    rerenderCurrentSelection();
}

QPoint MessageBox::calcSelectionPtrPosition(int btnIdx) {
    auto targetBtn = buttons[btnIdx];
    QPoint buttonCenter = {targetBtn->x() + targetBtn->width() / 2, targetBtn->y() + targetBtn->height() / 2};
    QPoint buttonLeftAlign = {buttonCenter.x() - targetBtn->calcSingleLineHorizAdvance() / 2, buttonCenter.y()};
    QPoint selectionPtrLocation = {buttonLeftAlign.x() - 5 - selectionPtr->width(), buttonLeftAlign.y() - selectionPtr->height() / 2};
    return selectionPtrLocation;
}

void MessageBox::rerenderCurrentSelection()
{
    // Set the selection underline
    for (int i = 0; i < numVisibleButtons; i++) {
        QFont buttonFont = buttons[i]->font();
        if (i == selectedIndex) {
            if (!buttonFont.underline()) {
                buttonFont.setUnderline(true);
                buttons[i]->setFont(buttonFont);
            }
        }
        else {
            if (buttonFont.underline()) {
                buttonFont.setUnderline(false);
                buttons[i]->setFont(buttonFont);
            }
        }
    }

    // Move selection pointer to its target position
    selectionPtr->move(calcSelectionPtrPosition(selectedIndex));
}

} // namespace WingletUI
