#include "saoitf_host.h"
#include "saoitf_constants.h"
#include "smbus.h"

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>


// ========================================
// Config Constants
// ========================================

#define ENABLE_DBG_PRINT 0

#define CMD_RETRY_CNT 3

// Out of SMBus reserved region, and before I2C EEPROMs
#define ARP_FIRST_ADDR 0x10
#define ARP_LAST_ADDR 0x47

#if ENABLE_DBG_PRINT
#include <stdio.h>
#define dbg_xfer_print(fmt, ...) fprintf(stderr, "[%s] " fmt "\n", __func__, ## __VA_ARGS__)
#define dbg_xfer_print_retry(fmt, ...) fprintf(stderr, "[%s] " fmt " (Try %d)\n", __func__, ## __VA_ARGS__, retry + 1)
#define dbg_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbg_xfer_print(fmt, ...)
#define dbg_xfer_print_retry(fmt, ...)
#define dbg_print(...)
#endif

// ========================================
// SMBus Function Extensions
// ========================================

int32_t i2c_smbus_read_block_data_exp_len(int file, uint8_t command, uint8_t exp_len, uint8_t *values) {
    if (exp_len > I2C_SMBUS_BLOCK_MAX)
        exp_len = I2C_SMBUS_BLOCK_MAX;

    union i2c_smbus_data data;
    int i, err;

    err = i2c_smbus_read_i2c_block_data(file, command, exp_len+1, data.block);
    if (err < 0)
        return err;

    // Make sure length matches
    if (data.block[0] != exp_len) {
        errno = ENOMEM;
        return -errno;
    }

    for (i = 1; i <= exp_len; i++)
        values[i-1] = data.block[i];
    return 0;
}

static bool smbus_set_addr(saoitf_host_state_t *state, uint8_t addr) {
    int ret = ioctl(state->fd, I2C_SLAVE, addr);
    if (ret < 0) {
      dbg_xfer_print("Failed to set i2c addr: %d", errno);
      return false;
    }
    return true;
}

// ========================================
// Low-Level Transfer Definitions
// ========================================

static void arp_issue_general_call(saoitf_host_state_t* state) {
    if (!smbus_set_addr(state, SMBUS_ADDR_GENERAL_CALL))
        return;
    i2c_smbus_write_byte(state->fd, GENERAL_CALL_RESET_CMD);
}

static bool arp_issue_prepare_to_arp(saoitf_host_state_t* state) {
    if (!smbus_set_addr(state, SMBUS_ADDR_ARP))
        return false;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int ret = i2c_smbus_write_byte(state->fd, ARP_CMD_PREPARE_TO_ARP);
        if (ret == 0)
            return true;
        dbg_xfer_print_retry("Failed to write: %d", ret);
    }
    return false;
}

static bool arp_issue_reset_device(saoitf_host_state_t* state) {
    if (!smbus_set_addr(state, SMBUS_ADDR_ARP))
        return false;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int ret = i2c_smbus_write_byte(state->fd, ARP_CMD_RESET_DEVICE);
        if (ret == 0)
            return true;
        dbg_xfer_print_retry("Failed to write: %d", ret);
    }
    return false;
}

static bool arp_get_next_udid_and_addr(saoitf_host_state_t* state, uint8_t* udid_and_addr_buf) {
    if (!smbus_set_addr(state, SMBUS_ADDR_ARP))
        return false;

    int ret = i2c_smbus_read_block_data_exp_len(state->fd, ARP_CMD_GET_UDID, ARP_LEN_UDID_AND_ADDR, udid_and_addr_buf);
    // If no device was present, it would've NAKed or read a length of 0xFF
    if (ret) {
        dbg_xfer_print("Failed to get next UDID: %d", ret);
        return false;
    }

    return true;
}

static bool arp_assign_addr(saoitf_host_state_t* state, uint8_t* udid_and_addr_buf) {
    if (!smbus_set_addr(state, SMBUS_ADDR_ARP))
        return false;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int ret = i2c_smbus_write_block_data(state->fd, ARP_CMD_ASSIGN_ADDRESS, ARP_LEN_UDID_AND_ADDR, udid_and_addr_buf);
        if (ret == 0)
            return true;
        dbg_xfer_print_retry("Failed to write: %d", ret);
    }
    return false;
}


// ========================================
// Device/Bubble up drivers
// ========================================

static bool saoitf_check_magic(saoitf_host_state_t* state, uint8_t addr) {
    if (!smbus_set_addr(state, addr))
        return false;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int32_t ret = i2c_smbus_read_word_data(state->fd, SAOITF_CMD_MAGIC);
        if (ret == SAOITF_MAGIC_WORD)
            return true;
        dbg_xfer_print_retry("Invalid Magic: %d", ret);
    }
    return false;
}

static bool saoitf_check_version(saoitf_host_state_t* state, uint8_t addr, uint16_t version) {
    if (!smbus_set_addr(state, addr))
        return false;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int32_t ret = i2c_smbus_read_word_data(state->fd, SAOITF_CMD_VERSION);
        if (ret >= 0 && (ret & SAOITF_VERSION_COMPAT_MASK) == (version & SAOITF_VERSION_COMPAT_MASK))
            return true;
        dbg_xfer_print_retry("Invalid Version: %d", ret);
    }
    return false;
}

static bool saoitf_get_vidpid(saoitf_host_state_t* state, uint8_t addr, uint16_t *vid_out, uint16_t *pid_out, uint16_t *fw_version_out) {
    if (!smbus_set_addr(state, addr))
        return false;

    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t fw_version = 0;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int32_t ret = i2c_smbus_read_word_data(state->fd, SAOITF_CMD_VID);
        if (ret > 0) {
            vid = ret;
            break;
        }
        dbg_xfer_print_retry("Invalid VID Read: %d", ret);
    }
    if (!vid)
        return false;

    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int32_t ret = i2c_smbus_read_word_data(state->fd, SAOITF_CMD_PID);
        if (ret > 0) {
            pid = ret;
            break;
        }
        dbg_xfer_print_retry("Invalid PID Read: %d", ret);
    }
    if (!pid)
        return false;

    bool saw_fw_version = false;
    for (int retry = 0; retry < CMD_RETRY_CNT; retry++) {
        int32_t ret = i2c_smbus_read_word_data(state->fd, SAOITF_CMD_FW_VERSION);
        if (ret >= 0) {
            fw_version = ret;
            saw_fw_version = true;
            break;
        }
        dbg_xfer_print_retry("Invalid FW Version Read: %d", ret);
    }
    if (!saw_fw_version)
        return false;

    *vid_out = vid;
    *pid_out = pid;
    *fw_version_out = fw_version;
    return true;

}

static bool saoitf_discover_device(saoitf_host_state_t* state, uint8_t addr) {
    // Make sure this implements the SAOITF protocol
    if (!saoitf_check_magic(state, addr)) {
        dbg_print("Invalid SAOITF Device: Bad magic\n");
        return false;
    }
    if (!saoitf_check_version(state, addr, SAOITF_VERSION_R0)) {
        dbg_print("Invalid SAOITF Device: Bad version\n");
        return false;
    }

    uint16_t vid, pid, fw_version;
    if (!saoitf_get_vidpid(state, addr, &vid, &pid, &fw_version)) {
        dbg_print("Invalid SAOITF Device: Could not read VID/PID\n");
        return false;
    }

    dbg_print("Found SAO with VID:PID = %04X:%04X (assigned address 0x%02X)\n", vid, pid, addr);
    bool found = false;
    if (state->discovery_cb)
        found = state->discovery_cb(state->cb_arg, addr, vid, pid, fw_version);

    if (!found) {
        dbg_print("Failed to find driver for saoitf device\n");
    }
    return found;
}


// ========================================
// Exported Functions
// ========================================

void saoitf_host_reset(saoitf_host_state_t* state) {
    // Forces re-discovery
    state->arp_okay = false;

    if (state->reset_cb)
        state->reset_cb(state->cb_arg);
}

void saoitf_scan_bus(saoitf_host_state_t* state) {
    // Reset Handling
    if (!state->arp_okay) {
        if (!arp_issue_reset_device(state)) {
            dbg_print("[ARP Assign] Failed to reset ARP devices\n");
            state->arp_okay = false;
            return;
        }
        arp_issue_general_call(state);
        if (!arp_issue_prepare_to_arp(state)) {
            dbg_print("[ARP Assign] Failed to issue prepare to ARP\n");
            state->arp_okay = false;
            return;
        }

        if (state->reset_cb)
            state->reset_cb(state->cb_arg);

        // Reset struct state
        state->arp_okay = true;
        state->next_arp_addr = ARP_FIRST_ADDR;
    }

    // Scan until we run out of addresses or don't find any more
    while (state->next_arp_addr <= ARP_LAST_ADDR) {
        uint8_t udid_addr[ARP_LEN_UDID_AND_ADDR];
        // No device found, break out
        if (!arp_get_next_udid_and_addr(state, udid_addr))
            break;

        // Assign the new device
        uint8_t device_addr = state->next_arp_addr;
        udid_addr[ARP_LEN_UDID] = (device_addr << 1) | 1;
        if (!arp_assign_addr(state, udid_addr)) {
            dbg_print("[ARP Assign] Failed to assign address to device\n");
            // Couldn't assign after a few attempts
            // Assuming the device disappeared from the bus, release it from the pool
            // NOTE: This behavior might not work for general call devices
            continue;
        }
        state->next_arp_addr++;

        uint16_t device_vid = ((uint16_t)udid_addr[2] << 8) | udid_addr[3];
        uint16_t device_devid = ((uint16_t)udid_addr[4] << 8) | udid_addr[5];

        if (device_vid != ARP_UDID_VID_SAOITF) {
            dbg_print("[ARP Assign] Unknown Device VID: %04X\n", device_vid);
        }
        else {
            // Defcon SAO
            bool saov3_device = false;
            switch (device_devid) {
            case ARP_UDID_DEVID_SAOITF_NONCOMPLIANT:
                dbg_print("[ARP Assign] Non-compliant device (ignoring right now)\n");
                break;
            case ARP_UDID_DEVID_SAOITF_GEN_CALL:
                dbg_print("[ARP Assign] General-call based device\n");
                saov3_device = true;
                break;
            case ARP_UDID_DEVID_SAOITF_FULL_ARP:
                dbg_print("[ARP Assign] Full ARP device discovered\n");
                saov3_device = true;
                break;
            default:
                dbg_print("[ARP Assign] Unknown Defcon device ID: %04X", device_devid);
            }

            if (saov3_device) {
                saoitf_discover_device(state, device_addr);
            }
        }
    }
}
