QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AVQtWidgets/RotaryMenuEntries/qclockmenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qcompassmenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qflightboardmenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qmediaplayermenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qoscopemenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qradarscopemenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qsplashmenuentry.cpp \
    AVQtWidgets/RotaryMenuEntries/qtestmenuentry.cpp \
    AVQtWidgets/clock.cpp \
    AVQtWidgets/compass.cpp \
    AVQtWidgets/flightboard.cpp \
    AVQtWidgets/getdata.cpp \
    AVQtWidgets/oscope.cpp \
    AVQtWidgets/qrotarymenu.cpp \
    AVQtWidgets/qrotarymenuentry.cpp \
    AVQtWidgets/radarscope.cpp \
    AVQtWidgets/simplemediaplayer.cpp \
    AVQtWidgets/splash.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    AVQtWidgets/RotaryMenuEntries/qclockmenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qcompassmenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qflightboardmenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qmediaplayermenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qoscopemenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qradarscopemenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qsplashmenuentry.h \
    AVQtWidgets/RotaryMenuEntries/qtestmenuentry.h \
    AVQtWidgets/clock.h \
    AVQtWidgets/compass.h \
    AVQtWidgets/flightboard.h \
    AVQtWidgets/getdata.h \
    AVQtWidgets/oscope.h \
    AVQtWidgets/qrotarymenu.h \
    AVQtWidgets/qrotarymenuentry.h \
    AVQtWidgets/radarscope.h \
    AVQtWidgets/simplemediaplayer.h \
    AVQtWidgets/splash.h \
    mainwindow.h

FORMS += \
    AVQtWidgets/clock.ui \
    AVQtWidgets/compass.ui \
    AVQtWidgets/radarscope.ui \
    AVQtWidgets/simplemediaplayer.ui \
    AVQtWidgets/splash.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    aerospace-village-logo.png

RESOURCES += \
    AVQtWidgets/files.qrc \
    resources.qrc
