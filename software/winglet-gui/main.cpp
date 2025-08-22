#include "wingletgui.h"

#include <QApplication>
#include <QProcessEnvironment>

#include "winglet-ui/theme.h"
#include "version_autogen.h"

const char* const WINGLET_GUI_VERSION = GIT_VERSION;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qputenv("XDG_DATA_HOME", "/var/apps");
    // Set UI thread to high priority priority for smoother animations
    QThread::currentThread()->setPriority(QThread::HighPriority);


    WingletUI::initializeThemes();
    WingletGUI w;
    a.setFont(QFont(WingletUI::activeTheme->standardFont));
    a.installEventFilter(&w);
    w.show();
    return a.exec();
}
