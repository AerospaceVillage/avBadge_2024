QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG(emulate_wpactrl) {
    DEFINES += NO_HARDWARE=1
}
else{
    LIBS += -lwpa_client -lgpiod
}

CONFIG += c++11
QMAKE_CXXFLAGS_RELEASE += -O3

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Add version info
version_target.target = version_autogen.h
version_target.depends = FORCE
version_target.commands =  @$$_PRO_FILE_PWD_/gen_version.sh \"$$OUT_PWD/$(DESTDIR_TARGET)/$$version_target.target\"
QMAKE_EXTRA_TARGETS += version_target
PRE_TARGETDEPS += $$version_target.target
# Need to create initial file so qmake will find this in the include path
system(touch "$$OUT_PWD/$$version_target.target")

SOURCES += \
    main.cpp \
    rgbleds.c \
    winglet-ui/hardware/brightnesscontrol.cpp \
    winglet-ui/hardware/canardinterface.cpp \
    winglet-ui/hardware/gpiocontrol.cpp \
    winglet-ui/hardware/ledcontrol.cpp \
    winglet-ui/hardware/sao/saoitf_host.c \
    winglet-ui/hardware/sao/smbus.c \
    winglet-ui/hardware/usbrolecontrol.cpp \
    winglet-ui/model/appmenumodel.cpp \
    winglet-ui/model/appsettingsenumselectionmodel.cpp \
    winglet-ui/model/knownnetworksmodel.cpp \
    winglet-ui/model/settingsmenumodel.cpp \
    winglet-ui/model/timezonesettingmodel.cpp \
    winglet-ui/model/wifiscanmodel.cpp \
    winglet-ui/settings/appsettings.cpp \
    winglet-ui/settings/appsettingspropentry.cpp \
    winglet-ui/settings/canardsettings.cpp \
    winglet-ui/settings/fastchargesetting.cpp \
    winglet-ui/settings/rootpasswordsetting.cpp \
    winglet-ui/settings/timezonesetting.cpp \
    winglet-ui/settings/wifienablesetting.cpp \
    winglet-ui/theme.cpp \
    winglet-ui/widget/elidedlabel.cpp \
    winglet-ui/widget/menuitemwidget.cpp \
    winglet-ui/widget/scrollablemenu.cpp \
    winglet-ui/widget/statusbar.cpp \
    winglet-ui/window/canardboard.cpp \
    winglet-ui/window/clock.cpp \
    winglet-ui/window/compass.cpp \
    winglet-ui/window/credits.cpp \
    winglet-ui/window/flightboard.cpp \
    winglet-ui/window/gpsboard.cpp \
    winglet-ui/window/gpstracker.cpp \
    winglet-ui/window/imagescreen.cpp \
    winglet-ui/window/mapscope.cpp \
    winglet-ui/window/oscope.cpp \
    winglet-ui/window/radarscope.cpp \
    winglet-ui/window/scrollarea.cpp \
    winglet-ui/window/settingsmenu.cpp \
    winglet-ui/window/simplemediaplayer.cpp \
    winglet-ui/windowcore/circularkeyboard.cpp \
    winglet-ui/windowcore/infoviewer.cpp \
    winglet-ui/windowcore/mainmenu.cpp \
    winglet-ui/windowcore/messagebox.cpp \
    winglet-ui/windowcore/selectorbox.cpp \
    winglet-ui/worker/abstractsocketworker.cpp \
    winglet-ui/worker/adsbreceiver.cpp \
    winglet-ui/worker/battmonitor.cpp \
    winglet-ui/worker/gpsreceiver.cpp \
    winglet-ui/worker/saomonitor.cpp \
    winglet-ui/worker/wifimonitor.cpp \
    wingletgui.cpp

HEADERS += \
    rgbleds.h \
    winglet-ui/hardware/brightnesscontrol.h \
    winglet-ui/hardware/canardinterface.h \
    winglet-ui/hardware/gpiocontrol.h \
    winglet-ui/hardware/ledcontrol.h \
    winglet-ui/hardware/sao/canard_constants.h \
    winglet-ui/hardware/sao/saoitf_constants.h \
    winglet-ui/hardware/sao/saoitf_host.h \
    winglet-ui/hardware/sao/smbus.h \
    winglet-ui/hardware/usbrolecontrol.h \
    winglet-ui/model/appmenumodel.h \
    winglet-ui/model/appsettingsenumselectionmodel.h \
    winglet-ui/model/knownnetworksmodel.h \
    winglet-ui/model/settingsmenumodel.h \
    winglet-ui/model/timezonesettingmodel.h \
    winglet-ui/model/wifiscanmodel.h \
    winglet-ui/settings/abstractsettingsentry.h \
    winglet-ui/settings/appsettings.h \
    winglet-ui/settings/appsettingspropentry.h \
    winglet-ui/settings/canardsettings.h \
    winglet-ui/settings/fastchargesetting.h \
    winglet-ui/settings/rootpasswordsetting.h \
    winglet-ui/settings/timezonesetting.h \
    winglet-ui/settings/wifienablesetting.h \
    winglet-ui/theme.h \
    winglet-ui/widget/elidedlabel.h \
    winglet-ui/widget/menuitemwidget.h \
    winglet-ui/widget/scrollablemenu.h \
    winglet-ui/widget/statusbar.h \
    winglet-ui/window/canardboard.h \
    winglet-ui/window/clock.h \
    winglet-ui/window/compass.h \
    winglet-ui/window/credits.h \
    winglet-ui/window/flightboard.h \
    winglet-ui/window/gpsboard.h \
    winglet-ui/window/gpstracker.h \
    winglet-ui/window/imagescreen.h \
    winglet-ui/window/mapscope.h \
    winglet-ui/window/oscope.h \
    winglet-ui/window/radarscope.h \
    winglet-ui/window/scrollarea.h \
    winglet-ui/window/settingsmenu.h \
    winglet-ui/window/simplemediaplayer.h \
    winglet-ui/windowcore/circularkeyboard.h \
    winglet-ui/windowcore/infoviewer.h \
    winglet-ui/windowcore/mainmenu.h \
    winglet-ui/windowcore/messagebox.h \
    winglet-ui/windowcore/selectorbox.h \
    winglet-ui/worker/abstractsocketworker.h \
    winglet-ui/worker/adsbreceiver.h \
    winglet-ui/worker/battmonitor.h \
    winglet-ui/worker/gpsreceiver.h \
    winglet-ui/worker/saomonitor.h \
    winglet-ui/worker/wifimonitor.h \
    winglet-ui/worker/workerthread.h \
    wingletgui.h

FORMS += \
    winglet-ui/window/credits.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /var/apps/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    assets/NOMAP.jpg \
    assets/style.json \
    aerospace-village-logo.png \
    assets/style_dark.json

RESOURCES += \
    resources.qrc

yourset.path = /var/apps/$${TARGET}/style/
yourset.files = assets/style.json

yourset1.path = /var/apps/$${TARGET}/style/
yourset1.files = assets/style_dark.json

INSTALLS += yourset yourset1
