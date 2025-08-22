# This file is part of buildroot-submodule.
#
#    buildroot-submodule is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    buildroot-submodule is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with buildroot-submodule.  If not, see <http://www.gnu.org/licenses/>. 2

# location of the project variant directory
PROJECT_VARIANT_DIR=$(CURDIR)/$(PROJECT_NAME)

#location of the buildroot sources
MAKEARGS := -C $(CURDIR)/buildroot 
#location to store build files
MAKEARGS += O=$(PROJECT_VARIANT_DIR)/output
# location to store extra config options and buildroot packages
MAKEARGS += BR2_EXTERNAL=$(CURDIR)
#transmit project name to be able to use it in kconfig
MAKEARGS += PROJECT_NAME=$(PROJECT_NAME)
# transmit project variant directory so it can be used in kconfig
MAKEARGS += PROJECT_VARIANT_DIR=$(PROJECT_VARIANT_DIR)
# location of default defconfig
DEFCONFIG_FILE=$(PROJECT_VARIANT_DIR)/defconfig
DEFCONFIG := BR2_DEFCONFIG=$(DEFCONFIG_FILE)

MAKEFLAGS += --no-print-directory

#these targets change the config file
config_change_targets:=menuconfig nconfig xconfig gconfig oldconfig \
       	silentoldconfig randconfig allyesconfig allnoconfig randpackageconfig \
       	allyespackageconfig allnopackageconfig

special_target:=$(config_change_targets) Makefile defconfig savedefconfig %_defconfig

all	:= $(filter-out $(special_target),$(MAKECMDGOALS))

default:  
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) defconfig
	$(MAKE) $(MAKEARGS) $(DEFCONFIG)


.PHONY: $(special_target) $(all) 

# Force runs defconfig on buildroot (useful before calling a specific target)
defconfig:
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) defconfig

# generate from a defconfig then save as current configuration
%_defconfig:
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) $@
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) savedefconfig
	$(call UPDATE_DEFCONFIG)


# update from current configuration, run the command, then save the result
$(config_change_targets): $(DEFCONFIG_FILE)
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) defconfig $@
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) savedefconfig

_all:
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) $(all)

$(all): _all
	@:

%/: _all
	@:

Makefile:;

$(DEFCONFIG_FILE):
	$(call UPDATE_DEFCONFIG)

define UPDATE_DEFCONFIG
	echo 'BR2_DL_DIR="$$(BR2_EXTERNAL_BUILDROOT_SUBMODULE_PATH)/dl"' >> $(DEFCONFIG_FILE)
	echo 'BR2_ROOTFS_OVERLAY="$$(BR2_EXTERNAL_BUILDROOT_SUBMODULE_PATH)/overlay"' >> $(DEFCONFIG_FILE)
	echo 'BR2_PACKAGE_OVERRIDE_FILE="$$(BR2_EXTERNAL_BUILDROOT_SUBMODULE_PATH)/local.mk"' >> $(DEFCONFIG_FILE)
	echo 'BR2_GLOBAL_PATCH_DIR="$$(BR2_EXTERNAL_BUILDROOT_SUBMODULE_PATH)/patch"' >> $(DEFCONFIG_FILE)
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) defconfig
	$(MAKE) $(MAKEARGS) $(DEFCONFIG) savedefconfig
endef


