#ifndef INFOVIEWER_H
#define INFOVIEWER_H

#include <QLabel>
#include <QWidget>
#include "winglet-ui/worker/gpsreceiver.h"

namespace WingletUI {

class InfoViewer : public QWidget
{
    Q_OBJECT
public:
    explicit InfoViewer(QWidget *parent = nullptr);
    QMap<QString, QString>infoMap;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLabel* avLogoLabel;
    QString labels[4] = {
        "OS Name", "OS Version", "Build Type", "Device Model"
    };

    QString values[4] = {
        "Winglet-OS", "1.1", "Debug", "TEST"
    };

    void buildInfoMap();

signals:

};

} //end  WingletUI namespace

#endif // INFOVIEWER_H
