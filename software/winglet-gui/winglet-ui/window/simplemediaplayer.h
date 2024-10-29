#ifndef SIMPLEMEDIAPLAYER_H
#define SIMPLEMEDIAPLAYER_H

#include <QWidget>
#include <QLabel>

namespace WingletUI {

class SimpleMediaPlayer : public QWidget
{
    Q_OBJECT
public:
    explicit SimpleMediaPlayer(QWidget *parent = nullptr);
    ~SimpleMediaPlayer();

protected:
    void hideEvent(QHideEvent *ev) override;
    void wheelEvent(QWheelEvent *ev) override;
    void keyPressEvent(QKeyEvent *ev) override;

private slots:
    void shimmerIncrement();

private:
    QLabel *label;
    QMovie *movie = nullptr;
    int shimmer_angle = 0;
    int movIndx = 0;
    void startMovie();
};

} // namespace WingletUI

#endif // SIMPLEMEDIAPLAYER_H
