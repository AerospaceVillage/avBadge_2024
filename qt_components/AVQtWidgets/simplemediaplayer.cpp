#include "simplemediaplayer.h"
#include "ui_simplemediaplayer.h"



simplemediaplayer::simplemediaplayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::simplemediaplayer)
{
    ui->setupUi(this);
    movie = new QMovie(":/file/assets/volleyball.gif");
    movie->setScaledSize(QSize(480,480));
    ui->label->setMovie(movie);
    movie->start();

}

simplemediaplayer::~simplemediaplayer()
{
    delete ui;
}
