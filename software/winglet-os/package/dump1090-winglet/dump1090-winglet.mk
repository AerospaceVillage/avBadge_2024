#############################################################
#
# dump1090-winglet
#
#############################################################
DUMP1090_WINGLET_VERSION = 0c3bb23eb447d4ae47c7013346fa6fa97482bb1d
DUMP1090_WINGLET_SITE_METHOD = git
DUMP1090_WINGLET_SITE = https://github.com/antirez/dump1090.git
DUMP1090_WINGLET_LICENSE = BSD

define DUMP1090_WINGLET_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
	$(TARGET_STRIP) $(@D)/dump1090
endef

define DUMP1090_WINGLET_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dump1090 $(TARGET_DIR)/usr/bin/dump1090
endef

$(eval $(generic-package))
