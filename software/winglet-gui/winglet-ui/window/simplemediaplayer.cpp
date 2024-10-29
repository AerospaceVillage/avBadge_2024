#include "simplemediaplayer.h"

#include <QKeyEvent>
#include <QWheelEvent>
#include <QMovie>
#include <QTimer>
#include "rgbleds.h"

#define MAXNUMMOVIES 5

namespace WingletUI {

SimpleMediaPlayer::SimpleMediaPlayer(QWidget *parent)
    : QWidget{parent}
{
    label = new QLabel(this);
    label->setGeometry(0, 0, 480, 480);

    QTimer* tir = new QTimer(this);
    connect(tir, SIGNAL(timeout()), this, SLOT(shimmerIncrement()));
    tir->start(100);

    startMovie();
}

SimpleMediaPlayer::~SimpleMediaPlayer()
{
    if (movie)
        delete movie;
    delete label;
}

void SimpleMediaPlayer::hideEvent(QHideEvent *ev)
{
    (void) ev;
    rgbled_clear();
}

void SimpleMediaPlayer::wheelEvent(QWheelEvent *ev) {

    if(ev->angleDelta().y() > 0){
        if(this->movIndx < MAXNUMMOVIES){
            movIndx = movIndx+1;
            startMovie();
        }
    } else {
        if(this->movIndx > 0){
            movIndx = movIndx-1;
            startMovie();
        }
    }
}

void SimpleMediaPlayer::keyPressEvent(QKeyEvent *ev) {
    switch (ev->key()) {
    case Qt::Key_Right:
        if(this->movIndx < MAXNUMMOVIES){
            movIndx = movIndx+1;
            startMovie();
        }
        break;
    case Qt::Key_Left:
        if(this->movIndx > 0){
            movIndx = movIndx-1;
            startMovie();
        }
        break;
    default:
        ev->ignore();
    }
}

void SimpleMediaPlayer::startMovie()
{
    QMovie *prevMovie = movie;
    switch (movIndx)
    {
    case 0:
    default:
        movie = new QMovie(":/videos/SpaceDebrie280.gif");
        break;
    case 1:
        movie = new QMovie(":/videos/Jupiter.gif");
        break;
    case 2:
        movie = new QMovie(":/videos/Earth.gif");
        break;
    case 3:
        movie = new QMovie(":/videos/volleyball.gif");
        break;
    case 4:
        movie = new QMovie(":/videos/DarkStar.gif");
        break;
    case 5:
        movie = new QMovie(":/videos/DBZ.gif");;
        break;
    }

    movie->setScaledSize(QSize(480, 480));
    label->setMovie(movie);
    movie->start();

    if (prevMovie)
        delete prevMovie;
}

void SimpleMediaPlayer::shimmerIncrement(){
    this->shimmer_angle += 10;
    if(this->shimmer_angle >= 360){
        this->shimmer_angle = this->shimmer_angle % 360;
    }
    rgbled_show_shimmer(this->shimmer_angle);
}

} // namespace WingletUI
