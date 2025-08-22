#############################################################
#
# led-daemon
#
#############################################################
LED_DAEMON_VERSION = 1
LED_DAEMON_SITE_METHOD = local
LED_DAEMON_SITE = $(LED_DAEMON_PKGDIR)/src

define LED_DAEMON_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define LED_DAEMON_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/ledDaemon_winglet $(TARGET_DIR)/usr/bin/leddaemon
endef

$(eval $(generic-package))
