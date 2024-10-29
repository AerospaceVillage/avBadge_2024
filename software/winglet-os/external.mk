# Makefiles used by all subprojects
include $(sort $(wildcard $(BR2_EXTERNAL_WINGLET_OS_PATH)/package/*/*.mk))

# Override FBV URL since the one in our buildroot version is broken
FBV_SITE = $(call github,amadvance,fbv,$(FBV_VERSION))
