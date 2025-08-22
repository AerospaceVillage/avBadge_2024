#include "theme.h"

#include <QWidget>
#include <QFontDatabase>
#include <QPainter>
#include <QElapsedTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include "winglet-ui/theme.h"

namespace WingletUI {

WingletTheme* activeTheme = nullptr;

WingletTheme::WingletTheme()
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

    bool startInDarkMode = true;
    m_inDarkMode = !startInDarkMode;  // Set boolean to opposite so setColorPalette will run
    setColorModePalette(startInDarkMode);

    animationDuration = 150;
}

void WingletTheme::setColorModePalette(bool darkMode) {
    if (m_inDarkMode == darkMode)
        return;

    if (darkMode) {
        palette.setColor(QPalette::HighlightedText, QColor("#0a7985"));
        palette.setColor(QPalette::Text, QColor("#ffac11"));
        palette.setColor(QPalette::Link, QColor("#db1675"));
        palette.setColor(QPalette::Window, QColor("#111111"));
        palette.setColor(QPalette::Shadow, QColor("#000000"));
        palette.setColor(QPalette::WindowText, QColor("#999999"));
        palette.setColor(QPalette::AlternateBase, QColor("#FF0000"));

        radar_sweep_lines_color = new QColor(57,255,20);
        radar_sweep_cursor_color = new QColor(220, 220, 220);
        radar_sweep_cursor_color_inv = new QColor(139,246,136);

        oscope_line_color = new QColor(255,255,0);

        palette.setColor(QPalette::Base, QColor("#36454F"));
        palette.setColor(QPalette::Midlight, QColor("#36454F"));

        /// If you want white maps delete the style.json
        ///    Default is white mode for the plugin
        ///
        /// If you wnat dark maps copy the style_dark.json into style.json
        ///
        /// For the Badge the default style.json is Dark Maps
        ///

        QString targetDirPath("/var/apps/winglet-gui/style/");
        QString targetFile("style.json");
        QString sourceResourcePath("style_dark.json");
        QString targetFilePath("style.json");
        // 1. Ensure the target directory exists
        QDir dir(targetDirPath);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) { // Create the directory, '.' refers to the path in QDir constructor
                qCritical() << "Failed to create directory for style file:" << targetDirPath;
            }
        }

        // 2. Remove any existing style.json at the target location
        // this will remove style.json which will make the maps white
        if (QFile::exists(dir.filePath(targetFile))) {
            //do nothing
        }

        // 3. Copy the selected theme file from resources to the target location
        // this will cp style_dark.json to style.json which will make the maps dark
        if (!QFile::copy(dir.filePath(sourceResourcePath), dir.filePath(targetFilePath))) {
        }

    }
    else {
        palette.setColor(QPalette::HighlightedText, QColor("#036570"));
        palette.setColor(QPalette::Text, QColor("#cc8808"));
        palette.setColor(QPalette::Link, QColor("#ba075e"));
        palette.setColor(QPalette::Window, QColor("#eeeeee"));
        palette.setColor(QPalette::Shadow, QColor("#dddddd"));
        palette.setColor(QPalette::WindowText, QColor("#111111"));

        radar_sweep_lines_color = new QColor("#006400");
        radar_sweep_cursor_color = new QColor("000000");
        radar_sweep_cursor_color_inv = new QColor(220, 220, 220);

        oscope_line_color = new QColor(Qt::darkBlue);

        palette.setColor(QPalette::Base, QColor("#A9A9A9"));
        palette.setColor(QPalette::Midlight, QColor("#A9A9A9"));

        QString targetDirPath("/var/apps/winglet-gui/style/");
        QString targetFile("style.json");
        QString sourceResourcePath("style_dark.json");
        QString targetFilePath("style.json");
        // 1. Ensure the target directory exists
        QDir dir(targetDirPath);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) { // Create the directory, '.' refers to the path in QDir constructor
                qCritical() << "Failed to create directory for style file:" << targetDirPath;
            }
        }

        // 2. Remove any existing style.json at the target location
        // this will remove style.json which will make the maps white
        if (QFile::exists(dir.filePath(targetFile))) {
            if (!QFile::remove(dir.filePath(targetFile))) {
            }
        }
    }


    loadMonochromeIcon(&avLogo, ":/images/av_logo.png", QPalette::Shadow);
    m_inDarkMode = darkMode;
    emit colorPaletteChanged();
}



void WingletTheme::renderBgAvLogo(QLabel *label)
{
    label->resize(avLogo.size());
    label->setPixmap(avLogo);
    label->lower();
    moveCenter(label, 300, 245);
}

void WingletTheme::renderBgAvLogo(QLabel *label, bool doLower)
{
    label->resize(avLogo.size());
    label->setPixmap(avLogo);
    if(doLower){
        label->lower();
    }
    moveCenter(label, 300, 245);
}

void WingletTheme::loadMonochromeIcon(QPixmap* pixmap, const QString& fileSrc, QPalette::ColorRole colorRole) {
    pixmap->load(fileSrc);
    QPainter painter(pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap->rect(), palette.color(colorRole));
    painter.end();
}

void initializeThemes() {
    if (activeTheme) delete activeTheme;
    activeTheme = new WingletTheme();
}

void moveCenter(QWidget* widget, int x, int y) {
    widget->move(x - widget->width() / 2, y - widget->height() / 2);
}

} // namespace WingletUI
