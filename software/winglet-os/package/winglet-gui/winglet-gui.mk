#############################################################
#
# winglet-gui
#
#############################################################
WINGLET_GUI_VERSION = ab84a7643f6cebae0e2101fcefc71834825eb877
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
