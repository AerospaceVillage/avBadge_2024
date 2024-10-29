#include "infoviewer.h"
#include "wingletgui.h"
#include "winglet-ui/theme.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QSysInfo>
#include <QMap>
#include <QNetworkInterface>

namespace WingletUI {

InfoViewer::InfoViewer(QWidget *parent)
    : QWidget{parent}
{
    this->buildInfoMap();
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel(this);
    title->setText("Badge Info");
    title->setForegroundRole(QPalette::Text);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont(activeTheme->titleFont, 26));
    layout->addWidget(title);

    //Iterate over the infoMap to display the information
    for (auto it = infoMap.keyValueBegin(); it != infoMap.keyValueEnd(); ++it) {
        //qDebug() << it->first << it->second;
        QLabel *x = new QLabel(this);
        x->setFont(QFont(activeTheme->standardFont, 12));
        x->setText(it->first + ": " + it->second);
        x->adjustSize();
        layout->addWidget(x);
    }

    layout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
    int margin = 100;   // 240 / sqrt(2);
    layout->setContentsMargins(QMargins(margin, margin, margin, margin));
    layout->setSpacing(-40);
    this->show();
}

void InfoViewer::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key::Key_A:
        case Qt::Key::Key_Return:
            break;
        case Qt::Key::Key_B:
            WingletGUI::inst->removeWidgetOnTop(this);
            break;
        default:
            event->ignore();
        }
}

void InfoViewer::buildInfoMap(){
    //Build out the key valu pairs to display
    infoMap.insert("OS Version", QSysInfo::prettyProductName());
    infoMap.insert("Kernel Version", QSysInfo::kernelVersion());
    infoMap.insert("Distro Base", QSysInfo::productType());
    infoMap.insert("Distro Version", QSysInfo::productVersion());

    //Get the Board Serial number
    QFile f("/proc/device-tree/serial-number");
    if(f.open(QFile::ReadOnly | QFile::Text)){
        QTextStream in(&f);
        QString serial = in.readLine();
        infoMap.insert("Device Serial", serial);
        f.close();

        //Get the Device Model type
        if(serial.indexOf("WA") == 0){
            infoMap.insert("Device Model", "Bring up Board");
        } else if(serial.indexOf("WB") == 0){
            infoMap.insert("Device Model", "Dev Kit");
        } else if(serial.indexOf("WC") == 0){
            infoMap.insert("Device Model", "Rev 1.1");
        }
    }


    //Get the Hash of the Image
    f.setFileName("/etc/os-release");
    if (f.open(QFile::ReadOnly | QFile::Text)){
        QTextStream in(&f);
        while (!in.atEnd()){
              QString line = in.readLine();
              int index = line.indexOf(QString("IMAGE_HASH="));
              if(index == 0){
                  infoMap.insert("OS Hash", line.mid(11,7));
              }
        }
        f.close();
     }


    //Get the ipv4 addresses of the connected interfaces
    foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces()){
        // Return only the first non-loopback MAC Address
        if (!(netInterface.flags() & QNetworkInterface::IsLoopBack)){
            QString interface = netInterface.humanReadableName();
            //qDebug() << "Interface: " << interface << "--------------------";
            for (const QNetworkAddressEntry &address: netInterface.addressEntries()) {
                //qDebug() << address.ip().toString();
                QString ipv4 = address.ip().toString();
                infoMap.insert(interface, ipv4);
                break;
            }
        }
    }
}

} // namespace WingletUI
