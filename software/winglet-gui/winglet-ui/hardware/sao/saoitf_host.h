/*
 * ===== SAO Host Interface Driver =====
 * Prototype
 */

#ifndef SAOITF_HOST_H_
#define SAOITF_HOST_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*saoitf_reset_handler_t)(void* arg);
typedef bool (*saoitf_discovery_handler_t)(void* arg, uint8_t addr, uint16_t vid, uint16_t pid, uint16_t fw_version);

typedef struct saoitf_host_state {
    int fd;
    bool arp_okay;
    uint8_t next_arp_addr;
    void* cb_arg;
    saoitf_reset_handler_t reset_cb;
    saoitf_discovery_handler_t discovery_cb;
} saoitf_host_state_t;

void saoitf_host_reset(saoitf_host_state_t* state);
void saoitf_scan_bus(saoitf_host_state_t* state);

// SMBus extensions
int32_t i2c_smbus_read_block_data_exp_len(int file, uint8_t command, uint8_t exp_len, uint8_t *values);

#ifdef __cplusplus
}
#endif

#endif
