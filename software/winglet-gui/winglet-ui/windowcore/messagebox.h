#ifndef WINGLETUI_MESSAGEBOX_H
#define WINGLETUI_MESSAGEBOX_H

#include <QWidget>
#include <QLabel>
#include <QParallelAnimationGroup>
#include "winglet-ui/widget/elidedlabel.h"

namespace WingletUI {

class MessageBox : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString messageText READ messageText WRITE setMessageText)
    Q_PROPERTY(QString titleText READ titleText WRITE setTitleText)

public:
    const static int MAX_BUTTONS = 3;

    explicit MessageBox(QWidget *parent = nullptr, Qt::Alignment alignment = Qt::AlignCenter);
    ~MessageBox();

    QString titleText() { return title->text(); }
    void setTitleText(const QString &text) { title->setText(text); }
    QString messageText() { return message->text(); }
    void setMessageText(const QString &text) { message->setText(text); }
    void setSingleButtonWithText(const QString &text);
    void setButtons(const QStringList &btnList, int newIdx = -1);
    void setSelectedIndex(int btnIdx);

signals:
    void buttonClicked(int btnIdx);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *ev) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void animationGroupFinished();

private:
    void rerenderButtonLabels();
    void rerenderCurrentSelection();
    QPoint calcSelectionPtrPosition(int btnIdx);

    void moveSelectionUp();
    void moveSelectionDown();

    enum AnimationState { ANIMATE_HIDDEN, ANIMATE_SHOWING, ANIMATE_SHOWN, ANIMATE_HIDING, ANIMATE_SELECTION } animationState;
    void animateShow();
    void animateHide();
    void animateSelection(int startIdx, int endIdx);
    void addOpacityAnimation(QWidget *widget, float start, float end);

    ElidedLabel* message;
    ElidedLabel* title;
    ElidedLabel* buttons[MAX_BUTTONS];
    QLabel* selectionPtr;
    QParallelAnimationGroup *animationGroup;

    int numVisibleButtons;
    int selectedIndex;
};

} // namespace WingletUI

#endif // WINGLETUI_MESSAGEBOX_H
