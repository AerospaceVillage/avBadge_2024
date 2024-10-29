#include "wingletgui.h"

#include <QApplication>
#include "winglet-ui/theme.h"
#include "rgbleds.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    int ret = rgbled_init();
//    if (ret)
//        perror("rgbled_init");

    WingletUI::initializeThemes();

    WingletGUI w;
    a.setFont(QFont(WingletUI::activeTheme->standardFont));
    a.installEventFilter(&w);
    w.show();
    return a.exec();
}
