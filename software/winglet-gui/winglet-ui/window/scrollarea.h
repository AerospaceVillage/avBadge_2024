#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include <QWidget>
#include <QScrollArea>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>
#include "winglet-ui/window/flightboard.h"
#include "winglet-ui/widget/statusbar.h"

namespace WingletUI {

class ScrollArea : public QWidget
{
    Q_OBJECT

private:

    FlightBoard *flightBoard;
    StatusBar* statusBar;
    QScrollArea *scrollArea;
    QLabel* titleLabel;
    QLabel *avLogoLabel;

public:
    explicit ScrollArea(QWidget *parent = nullptr);

    ~ScrollArea();

protected:
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *ev) override;
    void scrollWithDegrees(const QPoint &degrees);
    void setLeds();

};

} // namespace WingletUI

#endif // SCROLLAREA_H
