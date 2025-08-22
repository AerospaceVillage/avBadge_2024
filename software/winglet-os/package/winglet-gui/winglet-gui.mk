#############################################################
#
# winglet-gui
#
#############################################################
WINGLET_GUI_VERSION = 4316b12244350f9072c4e9ec9bdf23df1cda118f
WINGLET_GUI_SITE_METHOD = git
WINGLET_GUI_SITE = git@github.com:AerospaceVillage/winglet-gui.git

WINGLET_GUI_DEPENDENCIES = qt5base qtpbfimageplugin

define WINGLET_GUI_CONFIGURE_CMDS
	(cd $(@D); $(QT5_QMAKE))
endef

define WINGLET_GUI_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
	$(TARGET_STRIP) $(@D)/winglet-gui
endef

define WINGLET_GUI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/winglet-gui $(TARGET_DIR)/opt/winglet-gui/bin/winglet-gui
endef

$(eval $(generic-package))
