#############################################################
#
# QtPBFImagePlugin
#
#############################################################
QTPBFIMAGEPLUGIN_VERSION = 1d4640c85c97b0e21d00b5b2b6b5815889163c37
QTPBFIMAGEPLUGIN_SITE_METHOD = git
QTPBFIMAGEPLUGIN_SITE = https://github.com/tumic0/QtPBFImagePlugin.git
QTPBFIMAGEPLUGIN_DEPENDENCIES = qt5base
QTPBFIMAGEPLUGIN_INSTALL_STAGING = YES

$(eval $(qmake-package))
