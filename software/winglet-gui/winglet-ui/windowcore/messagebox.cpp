#include "winglet-ui/theme.h"
#include "messagebox.h"
#include "wingletgui.h"
#include <QKeyEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace WingletUI {

MessageBox::MessageBox(QWidget *parent, Qt::Alignment alignment)
    : QWidget{parent}, animationState(ANIMATE_HIDDEN),
      message(new QLabel(this)), title(new QLabel(this)),
      button(new QLabel(this)), selectionPtr(new QLabel(this)),
      animationGroup(new QParallelAnimationGroup(this))
{
    QGraphicsOpacityEffect* effect;

    title->setGeometry(0, 80, 480, 50);
    title->setForegroundRole(QPalette::Text);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont(activeTheme->titleFont, 26));
    effect = new QGraphicsOpacityEffect(title);
    effect->setOpacity(0);
    title->setGraphicsEffect(effect);

    message->setForegroundRole(QPalette::WindowText);
    message->setAlignment(alignment);
    message->setText("No Message");
    message->setFont(QFont(activeTheme->standardFont, 12));
    effect = new QGraphicsOpacityEffect(message);
    effect->setOpacity(0);
    message->setGraphicsEffect(effect);

    button->setGeometry(0, 420, 480, 30);
    button->setText("Okay");
    button->setForegroundRole(QPalette::HighlightedText);
    button->setAlignment(Qt::AlignCenter);
    button->setFont(QFont(activeTheme->standardFont, 16));
    effect = new QGraphicsOpacityEffect(button);
    effect->setOpacity(0);
    button->setGraphicsEffect(effect);

    // Create selection ptr
    selectionPtr->setText(">");
    selectionPtr->setForegroundRole(QPalette::HighlightedText);
    selectionPtr->setAlignment(Qt::AlignCenter);
    selectionPtr->setFont(QFont(activeTheme->titleFont, 16, QFont::Bold));
    selectionPtr->setFixedSize(selectionPtr->sizeHint());
    effect = new QGraphicsOpacityEffect(selectionPtr);
    effect->setOpacity(0);
    selectionPtr->setGraphicsEffect(effect);
    recomputePointerLocation();

    connect(animationGroup, SIGNAL(finished()), this, SLOT(animationGroupFinished()));
}

MessageBox::~MessageBox()
{
    delete animationGroup;
    delete message;
    delete title;
    delete button;
    delete selectionPtr;
}

void MessageBox::showEvent(QShowEvent *event)
{
    (void) event;
    animateShow();
}

void MessageBox::animationGroupFinished() {
    if (animationState == ANIMATE_SHOWING) {
        animationState = ANIMATE_SHOWN;
    }
    else if (animationState == ANIMATE_HIDING) {
        animationState = ANIMATE_HIDDEN;
        WingletGUI::inst->removeWidgetOnTop(this);
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
    addOpacityAnimation(button, 0.0, 1.0);
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
    addOpacityAnimation(button, 1.0, 0.0);
    addOpacityAnimation(selectionPtr, 1.0, 0.0);

    animationState = ANIMATE_HIDING;
    animationGroup->start();
}

void MessageBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key::Key_A:
    case Qt::Key::Key_Return:
    {
        // Add underline to button
        QFont buttonFont = button->font();
        buttonFont.setUnderline(true);
        button->setFont(buttonFont);
        button->setForegroundRole(QPalette::Link);
        selectionPtr->setForegroundRole(QPalette::Link);

        animateHide();
        break;
    }
    case Qt::Key::Key_B:
        break;
    default:
        event->ignore();
    }
}

void MessageBox::recomputeMsgLocation()
{
    message->ensurePolished();
    message->adjustSize();
    moveCenter(message, 480/2, 480/2);
}

void MessageBox::recomputePointerLocation()
{
    // Compute the selection pointer location
    QPoint buttonCenter = {button->x() + button->width() / 2, button->y() + button->height() / 2};
    QFontMetrics fm(button->font());
    QPoint buttonLeftAlign = {buttonCenter.x() - fm.horizontalAdvance(button->text()) / 2, buttonCenter.y()};
    QPoint selectionPtrLocation = {buttonLeftAlign.x() - 5 - selectionPtr->width(), buttonLeftAlign.y() - selectionPtr->height() / 2};
    selectionPtr->move(selectionPtrLocation);
}

} // namespace WingletUI
