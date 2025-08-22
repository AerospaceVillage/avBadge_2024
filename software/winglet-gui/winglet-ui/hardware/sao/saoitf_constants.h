#ifndef SAOITF_CONSTANTS_H_
#define SAOITF_CONSTANTS_H_


// ========================================
// SMBus General Constants
// ========================================

#define SMBUS_ADDR_GENERAL_CALL 0x00
#define SMBUS_ADDR_INVALID 0x7F
#define SMBUS_ADDR_ARP 0x61

#define GENERAL_CALL_RESET_CMD 0x01


// ========================================
// Address Resolution Protocol Constants
// ========================================

#define ARP_CMD_PREPARE_TO_ARP 0x01
#define ARP_CMD_RESET_DEVICE 0x02
#define ARP_CMD_GET_UDID 0x03
#define ARP_CMD_ASSIGN_ADDRESS 0x04
#define ARP_CMD_MIN_DIRADDR 0x20
#define ARP_CMD_MAX_DIRADDR 0xFD


#define ARP_LEN_UDID 16
#define ARP_LEN_UDID_AND_ADDR (ARP_LEN_UDID + 1)

#define ARP_UDID_CAP_FIXED_ADDR 0x00
#define ARP_UDID_CAP_DYN_PERSISTANT_ADDR 0x40
#define ARP_UDID_CAP_DYN_VOLATILE_ADDR 0x80
#define ARP_UDID_CAP_RANDOM_ADDR 0xC0
#define ARP_UDID_CAP_PEC_SUPPORTED 0x01

#define ARP_UDID_VERS_UDID1 0x04
// Revision field is used to hold SAOITF Version
// If we have more than 8 versions, then the DEVID should change
#define ARP_UDID_REVISION_SAOITF_V0 0x00

#define ARP_UDID_VID_SAOITF 0xDC5A
#define ARP_UDID_DEVID_SAOITF_NONCOMPLIANT 0x100
#define ARP_UDID_DEVID_SAOITF_GEN_CALL 0x101
#define ARP_UDID_DEVID_SAOITF_FULL_ARP 0x102

#define ARP_UDID_ITF_VERS_SMBUS1_0 0x00
#define ARP_UDID_ITF_VERS_SMBUS1_1 0x01
#define ARP_UDID_ITF_VERS_SMBUS2_0 0x04
#define ARP_UDID_ITF_VERS_SMBUS3_0 0x05

#define ARP_UDID_ITF_SUPPORT_OEM 0x10


// ========================================
// SAOITF Specific Constants
// ========================================

#define SAOITF_CMD_MAGIC 0x00
#define SAOITF_CMD_VERSION 0x01
#define SAOITF_CMD_VID 0x02
#define SAOITF_CMD_PID 0x03
#define SAOITF_CMD_FW_VERSION 0x04
#define SAOITF_CMD_MANUFACTUER 0x5
#define SAOITF_CMD_NAME 0x06
#define SAOITF_CMD_SERIAL_NUM 0x07
#define SAOITF_CMD_GPIO_CAPABILITIES 0x08
#define SAOITF_CMD_GPIO_MODE 0x09
#define SAOITF_CMD_ARP_RESET 0x0A
#define SAOITF_CMD_CLASS_INTERFACES 0x0B

#define SAOITF_CMD_CLASS_BASE 0x20
#define SAOITF_CMD_VENDOR_BASE 0x80


// Magic word = 'SA'
#define SAOITF_MAGIC_WORD 0x5341

// Version Words
// Note changing the lower 8 bits of a version keeps it compatible with older versions
//  - Allows adding on features without breaking things
// Changing the upper 8 bits of version breaks compatability
//  - THIS WILL CAUSE IT TO NOT WORK WITH OLDER BADGES
#define SAOITF_VERSION_COMPAT_MASK 0xFF00

#define SAOITF_VERSION_R0 0x0100


#endif
