#include "theme.h"

#include <QWidget>
#include <QFontDatabase>
#include <QPainter>

namespace WingletUI {

WingletTheme* activeTheme = nullptr;

WingletTheme::WingletTheme(): avLogo(":/images/av_logo.png")
{
    static int standardId = -1;
    static int titleId = -1;
    static int fancyId = -1;
    if (standardId == -1) {
        standardId = QFontDatabase::addApplicationFont(":/fonts/lato.ttf");
        QFontDatabase::addApplicationFont(":/fonts/lato-bold.ttf");
    }
    if (titleId == -1) {
        titleId = QFontDatabase::addApplicationFont(":/fonts/neuropol-x.ttf");
    }
    if (fancyId == -1) {
        fancyId = QFontDatabase::addApplicationFont(":/fonts/sofachrome.ttf");
    }

    standardFont = QFontDatabase::applicationFontFamilies(standardId).at(0);
    titleFont = QFontDatabase::applicationFontFamilies(titleId).at(0);
    fancyFont = QFontDatabase::applicationFontFamilies(fancyId).at(0);

    palette.setColor(QPalette::HighlightedText, QColor("#0a7985"));
    palette.setColor(QPalette::Text, QColor("#ffac11"));
    palette.setColor(QPalette::Link, QColor("#db1675"));
    palette.setColor(QPalette::Window, QColor("#111111"));
    palette.setColor(QPalette::Shadow, QColor("#000000"));
    palette.setColor(QPalette::WindowText, QColor("#999999"));

    animationDuration = 150;

    /*
    palette.setColor(QPalette::Window, QColor("#eeeeee"));
    palette.setColor(QPalette::Shadow, QColor("#dddddd"));
    palette.setColor(QPalette::WindowText, QColor("#111111")); // */
}

void WingletTheme::renderBgAvLogo(QLabel *label)
{
    // Color the avLogo with the shadow color
    QPixmap logoCopy = avLogo;
    QPainter painter(&logoCopy);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(logoCopy.rect(), activeTheme->palette.color(QPalette::Shadow));
    painter.end();
    label->resize(logoCopy.size());
    label->setPixmap(logoCopy);
    label->lower();
    moveCenter(label, 300, 245);
}

void initializeThemes() {
    if (activeTheme) delete activeTheme;
    activeTheme = new WingletTheme();
}

void moveCenter(QWidget* widget, int x, int y) {
    widget->move(x - widget->width() / 2, y - widget->height() / 2);
}

} // namespace WingletUI
