# Makefiles used by all subprojects
include $(sort $(wildcard $(BR2_EXTERNAL_WINGLET_OS_PATH)/package/*/*.mk))


define UBOOT_SAVE_GIT_HASH
	@cd "$(UBOOT_DL_DIR)/git" && rm -f ./.scmversion && ./scripts/setlocalversion --save-scmversion
	@bash -c 'tmpd="$$(mktemp -d "${BUILD_DIR}/.uboot-$(UBOOT_VERSION)$(BR_FMT_VERSION_git).tar.gz.XXXXXX")" && cd "$$tmpd" && . $(shell pwd)/support/download/helpers && mk_tar_gz "$(UBOOT_DL_DIR)/git" "uboot-$(UBOOT_VERSION)" "$(shell bash -c 'GIT_DIR="$(UBOOT_DL_DIR)/git/.git" git log -1 --pretty=format:%ci')" "$(UBOOT_DL_DIR)/uboot-$(UBOOT_VERSION)$(BR_FMT_VERSION_git).tar.gz" ".git/*" && rm -rf $$tmpd'
endef

define UBOOT_RSYNC_GEN_GIT_HASH
	@cd "$(UBOOT_DIR)" && rm -f ./.scmversion && ./scripts/setlocalversion --save-scmversion "$(UBOOT_OVERRIDE_SRCDIR)"
endef

UBOOT_POST_DOWNLOAD_HOOKS += UBOOT_SAVE_GIT_HASH
UBOOT_POST_RSYNC_HOOKS += UBOOT_RSYNC_GEN_GIT_HASH


define WINGLET_GUI_SAVE_GIT_HASH
	@cd "$(WINGLET_GUI_DL_DIR)/git" && ./gen_version.sh --save "$(WINGLET_GUI_DL_DIR)/git/.version_info"
	@bash -c 'tmpd="$$(mktemp -d "${BUILD_DIR}/.winglet-gui-$(WINGLET_GUI_VERSION)$(BR_FMT_VERSION_git).tar.gz.XXXXXX")" && cd "$$tmpd" && . $(shell pwd)/support/download/helpers && mk_tar_gz "$(WINGLET_GUI_DL_DIR)/git" "winglet-gui-$(WINGLET_GUI_VERSION)" "$(shell bash -c 'GIT_DIR="$(WINGLET_GUI_DL_DIR)/git/.git" git log -1 --pretty=format:%ci')" "$(WINGLET_GUI_DL_DIR)/winglet-gui-$(WINGLET_GUI_VERSION)$(BR_FMT_VERSION_git).tar.gz" ".git/*" && rm -rf $$tmpd'
endef

define WINGLET_GUI_RSYNC_GEN_GIT_HASH
	@cd "$(WINGLET_GUI_OVERRIDE_SRCDIR)" && ./gen_version.sh --save "$(WINGLET_GUI_DIR)/.version_info"
endef

WINGLET_GUI_POST_DOWNLOAD_HOOKS += WINGLET_GUI_SAVE_GIT_HASH
WINGLET_GUI_POST_RSYNC_HOOKS += WINGLET_GUI_RSYNC_GEN_GIT_HASH
