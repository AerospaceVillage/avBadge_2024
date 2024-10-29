#ifndef WINGLETUI_BASETHEME_H
#define WINGLETUI_BASETHEME_H

#include <QApplication>
#include <QPalette>
#include <QLabel>

namespace WingletUI {

enum ExitCode {
    EXIT_CODE_RESTART_UI = 0,
    EXIT_CODE_GAMEBOY = 40,
    EXIT_CODE_POWEROFF = 41,
    EXIT_CODE_REBOOT = 42,
    EXIT_CODE_RUN_TERMINAL = 43,
    EXIT_CODE_HIL = 44,
    EXIT_CODE_REGISTER_DISCORD = 45
};

class WingletTheme
{
public:
    WingletTheme();

    QPalette palette;

    QString standardFont;
    QString titleFont;
    QString fancyFont;
    int animationDuration;

    QPixmap avLogo;
    void renderBgAvLogo(QLabel *label);
};

void initializeThemes();
extern WingletTheme* activeTheme;

void moveCenter(QWidget* widget, int x, int y);

} // namespace WingletUI

#endif // WINGLETUI_BASETHEME_H
