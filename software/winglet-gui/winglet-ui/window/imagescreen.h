#ifndef IMAGESCREEN_H
#define IMAGESCREEN_H

#include <QWidget>
#include <QLabel>

namespace WingletUI {

class ImageScreen : public QWidget
{
    Q_OBJECT
public:
    explicit ImageScreen(QWidget *parent = nullptr);
    ~ImageScreen();

private:
    QLabel *label;
};

} // namespace WingletUI

#endif // IMAGESCREEN_H
