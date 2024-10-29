#include "imagescreen.h"

namespace WingletUI {

ImageScreen::ImageScreen(QWidget *parent)
    : QWidget{parent}
{
    label = new QLabel(this);
    label->setGeometry(0, 0, 480, 480);
    label->setAlignment(Qt::AlignCenter);
    label->setPixmap(QPixmap(":/images/alien_tesla.png"));
}

ImageScreen::~ImageScreen()
{
    delete label;
}

} // namespace WingletUI
