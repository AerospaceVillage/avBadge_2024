#ifndef WINGLETUI_MESSAGEBOX_H
#define WINGLETUI_MESSAGEBOX_H

#include <QWidget>
#include <QLabel>
#include <QParallelAnimationGroup>

namespace WingletUI {

class MessageBox : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString messageText READ messageText WRITE setMessageText)
    Q_PROPERTY(QString titleText READ titleText WRITE setTitleText)

public:
    explicit MessageBox(QWidget *parent = nullptr, Qt::Alignment alignment = Qt::AlignCenter);
    ~MessageBox();

    QString titleText() { return title->text(); }
    void setTitleText(const QString &text) { title->setText(text); }
    QString messageText() { return message->text(); }
    void setMessageText(const QString &text) { message->setText(text); recomputeMsgLocation(); }
    void setButtonText(const QString &text) { button->setText(text); recomputePointerLocation(); }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void animationGroupFinished();

private:
    void recomputeMsgLocation();
    void recomputePointerLocation();

    enum AnimationState { ANIMATE_HIDDEN, ANIMATE_SHOWING, ANIMATE_SHOWN, ANIMATE_HIDING } animationState;
    void animateShow();
    void animateHide();
    void addOpacityAnimation(QWidget *widget, float start, float end);

    QLabel* message;
    QLabel* title;
    QLabel* button;
    QLabel* selectionPtr;
    QParallelAnimationGroup *animationGroup;
};

} // namespace WingletUI

#endif // WINGLETUI_MESSAGEBOX_H
