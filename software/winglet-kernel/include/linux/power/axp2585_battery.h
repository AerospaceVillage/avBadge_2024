#ifndef AXP2585_CHARGER_H_
#define AXP2585_CHARGER_H_

#include <linux/platform_device.h>

enum axp2585_usb_id_mode {
	AXP2585_ID_PERIPHERAL,
	AXP2585_ID_HOST
};

typedef void (*usbc_state_update_cb_t)(void* arg);

struct platform_device *axp2585_batt_by_phandle(struct device *dev, int index);
void axp2585_batt_register_det_cb(struct platform_device *pdev,
					usbc_state_update_cb_t cb, void* arg);
int axp2585_batt_get_id_mode(struct platform_device *pdev,
				enum axp2585_usb_id_mode *mode_out);

#endif
