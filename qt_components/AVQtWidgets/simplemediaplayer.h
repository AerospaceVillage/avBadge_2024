#ifndef SIMPLEMEDIAPLAYER_H
#define SIMPLEMEDIAPLAYER_H

#include <QWidget>
//#include <QtMultimedia>
//#include <QtMultimediaWidgets>
#include <QMovie>

namespace Ui {
class simplemediaplayer;
}

class simplemediaplayer : public QWidget
{
    Q_OBJECT

public:
    explicit simplemediaplayer(QWidget *parent = nullptr);
    ~simplemediaplayer();
    QMovie *movie;
private:
    Ui::simplemediaplayer *ui;
};

#endif // SIMPLEMEDIAPLAYER_H
