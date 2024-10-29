#include <common.h>
#include <command.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <video.h>
#include <linux/delay.h>
#include <mapmem.h>
#include <backlight.h>
#include <dm/uclass-internal.h>

#define BMU_CHG_STATUS (0x00)
#define VBUS_GD_MASK (1<<1)
#define BMU_BAT_STATUS (0x02)
#define BAT_DET_OKAY (1<<4)
#define BAT_DET_PRESENT (1<<3)
#define AXP2585_CHIP_ID (0x03)
#define AXP2585_VERSION_MASK (0xCF)
#define AXP2585_VERSION_C (0x49)
#define AXP2585_VERSION_D (0x46)
#define BMU_BATFET_CTL (0x10)
#define BMU_BATFET_OFF_MASK (1<<7)
#define BMU_BOOST_EN (0x12)
#define BMU_BOOST_CTL (0x13)
#define PWR_ON_CTL (0x17)
#define BMU_POWER_ON_STATUS (0x4A)
#define BMU_BAT_VOL_H (0x78)
#define BMU_BAT_VOL_L (0x79)
#define BMU_CHG_CUR_LIMIT (0x8b)
#define BMU_BAT_PERCENTAGE (0xB9)
#define BMU_BAT_VOL_OCV_H (0xBC)
#define BMU_BAT_VOL_OCV_L (0xBD)
#define BMU_REG_LOCK (0xF2)
#define BMU_REG_EXTENSION_EN (0xF4)
#define BMU_ADDR_EXTENSION (0xFF)

#define BMU_IRQ_PWRBTN_EN (0x43)
#define BMU_IRQ_PWRBTN_STAT (0x4B)
#define BMU_PWRBTN_SHORT_MASK (1<<7)
#define BMU_PWRBTN_LONG_MASK (1<<6)
#define BMU_PWRBTN_POSEDGE_MASK (1<<5)
#define BMU_PWRBTN_NEGEDGE_MASK (1<<5)

#define axp_err(_fmt...) log_err("[axp2585] " _fmt)
#define axp_info(_fmt...) log_info("[axp2585] " _fmt)

static struct udevice *axp2585_bmu = NULL;
#ifdef CONFIG_AXP2585_FAKE_PWROFF
static int axp_do_fake_pwroff = 0;
#endif

static int axp2585_bootstrap(struct udevice *dev)
{
	u32 cmd_out;
	int i = 0;
	int ret = 0;

	while (!ofnode_read_u32_index(dev_ofnode(dev), "init-commands", i++, &cmd_out)) {
		u8 reg = (cmd_out >> 8) & 0xFF;
		u8 val = cmd_out & 0xFF;

		ret = dm_i2c_reg_write(dev, reg, val);
		if (ret) {
			axp_err("Failed bootstrap: Write to reg 0x%02X with val 0x%02X failed: %d\n",
				reg, val, ret);
			break;
		}
	}

	return ret;
}

int axp2585_check_fake_pwroff_requested(struct udevice *dev) {
	return dm_i2c_reg_read(dev, 0x94) == 0xA5;
}

void axp2585_set_fake_pwroff_requested(struct udevice *dev, int req) {
	// We use Breath function control4 reg to hold fake poweroff request
	// Isn't used by us and stays persistent in BMU
	// If needed, set to magic value 0xA5, else set to reset value of 0x01

	dm_i2c_reg_write(dev, 0x94, req ? 0xA5 : 0x01);
}

static int axp2585_probe(struct udevice *dev)
{
	u8 axp_chip_id;
	int ret = 0;

	ret = dm_i2c_reg_read(dev, AXP2585_CHIP_ID);
	if (ret < 0) {
		axp_err("Failed to read i2c chip id: %d\n", ret);
		return ret;
	}
	axp_chip_id = (ret & AXP2585_VERSION_MASK);
	
	if (axp_chip_id == AXP2585_VERSION_D) {
		debug("Discovered AXP2585 BMU (Ver. D)\n");
	} else if (axp_chip_id == AXP2585_VERSION_C) {
		debug("Discovered AXP2585 BMU (Ver. C)\n");
	} else {
		axp_err("Invalid BMU Chip ID: 0x%02X\n", axp_chip_id);
		return -EINVAL;
	}

	// Need to clear batfet on so devices can power off by just setting the bit
	ret = dm_i2c_reg_read(dev, BMU_BATFET_CTL);
	if (ret < 0) {
		axp_err("Failed to read batfet ctrl: %d\n", ret);
		return ret;
	}
	if (ret & BMU_BATFET_OFF_MASK) {
		// Batfet off still set from last time board was shut off
		// Need to clear that
		ret = ret & ~BMU_BATFET_OFF_MASK;
		ret = dm_i2c_reg_write(dev, BMU_BATFET_CTL, ret);
		if (ret < 0) {
			axp_err("Failed to clear batfet ctrl: %d\n", ret);
			return ret;
		}
	}

	ret = axp2585_bootstrap(dev);
	if (ret) {
		axp_err("Failed bmu bootstrap: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_AXP2585_FAKE_PWROFF
	axp_do_fake_pwroff = axp2585_check_fake_pwroff_requested(dev);
	if (axp_do_fake_pwroff) {
		// Disable splash screen when we do fake poweroff
		video_disable_splash(1);
	}
	axp2585_set_fake_pwroff_requested(dev, 0);
#endif

	axp2585_bmu = dev;
	return 0;
}

#ifdef CONFIG_AXP2585_FAKE_PWROFF
int axp2585_check_fake_pwroff(void) {
	int ret;
	u32 min_poweron_mv;

	if (!axp2585_bmu) return 0;

	ret = ofnode_read_u32(dev_ofnode(axp2585_bmu), "min-poweron-mv", &min_poweron_mv);
	if (ret) {
		axp_err("Failed to read 'min-poweron-mv': %d\n", ret);
		goto fakepoweroff_check;
	}


	// Get display so we can render things
	struct udevice *vdev;
	ret = uclass_find_first_device(UCLASS_VIDEO, &vdev);
	if (ret) {
		printf("[FAKE_PWROFF] Could not get display: %d\n", ret);
		vdev = NULL;
	}

	ret = dm_i2c_reg_read(axp2585_bmu, BMU_CHG_STATUS);
	int sys_plugged_in = (ret >= 0 && !!(ret & VBUS_GD_MASK));
	ret = dm_i2c_reg_read(axp2585_bmu, BMU_BAT_STATUS);
	const int batt_present_flags = BAT_DET_OKAY | BAT_DET_PRESENT;
	if (!sys_plugged_in && ret >= 0 && (ret & batt_present_flags) == batt_present_flags) {
		u32 batt_voltage_mv;
		ret = dm_i2c_reg_read(axp2585_bmu, BMU_BAT_VOL_H);
		if (ret < 0) goto fakepoweroff_check;
		batt_voltage_mv = ((u16)ret) << 4;
		ret = dm_i2c_reg_read(axp2585_bmu, BMU_BAT_VOL_L);
		if (ret < 0) goto fakepoweroff_check;
		batt_voltage_mv |= ret & 0xF;

		if (batt_voltage_mv == 0) {
			// My devkit sometimes returns 0?? Fall back to OCV if so
			ret = dm_i2c_reg_read(axp2585_bmu, BMU_BAT_VOL_OCV_H);
			if (ret < 0) goto fakepoweroff_check;
			batt_voltage_mv = ((u16)ret) << 4;
			ret = dm_i2c_reg_read(axp2585_bmu, BMU_BAT_VOL_OCV_L);
			if (ret < 0) goto fakepoweroff_check;
			batt_voltage_mv |= ret & 0xF;
		}

		batt_voltage_mv *= 1200;
		batt_voltage_mv /= 1000;

		if (batt_voltage_mv < min_poweron_mv) {
			printf("!!! BATTERY CRITICALLY LOW (%d mV) !!!\n", batt_voltage_mv);
			// Set icon to the usb powered shutdown mode
			video_clear(vdev);

			u8 *data = (u8*) video_get_batt_low_logo();
			ret = video_bmp_display(vdev, map_to_sysmem(data), CONFIG_VIDEO_LOGO_X, CONFIG_VIDEO_LOGO_Y, true);

			mdelay(5000);

			printf("!!! SHUTTING DOWN !!!\n");
			ret = dm_i2c_reg_clrset(axp2585_bmu, BMU_BATFET_CTL,
					BMU_BATFET_OFF_MASK, BMU_BATFET_OFF_MASK);
			if (ret < 0) goto fakepoweroff_check;
			mdelay(500);
			printf("Still Alive!?! Continuing in idle...\n");
			// Clear if we're still alive since if it's still stuck, it won't retrigger
			ret = dm_i2c_reg_clrset(axp2585_bmu, BMU_BATFET_CTL,
				BMU_BATFET_OFF_MASK, 0);
			if (ret < 0) goto fakepoweroff_check;
		}
	}

fakepoweroff_check:
	if (!axp_do_fake_pwroff) return 0;

	printf("[FAKE_PWROFF] Shutdown requested, but still plugged in. Entering fake power off mode\n");

	const u8 irq_mask = BMU_PWRBTN_LONG_MASK | BMU_PWRBTN_POSEDGE_MASK;
	ret = dm_i2c_reg_write(axp2585_bmu, BMU_IRQ_PWRBTN_STAT, irq_mask);
	if (ret) return 0;  // Couldn't clear, don't have comms

	struct udevice *bldev;
	struct backlight_ops* blops = NULL;
	ret = uclass_find_first_device(UCLASS_PANEL_BACKLIGHT, &bldev);
	if (ret) {
		printf("[FAKE_PWROFF] Could not get backlight: %d\n", ret);
		bldev = NULL;
	}
	else {
		blops = (struct backlight_ops*) bldev->driver->ops;
	}

	const int display_keepalive_tick_full = 50;
	int display_keepalive_ticks = display_keepalive_tick_full;
	if (vdev) {
		// Set icon to the usb powered shutdown mode
		video_clear(vdev);

		u8 *data = (u8*) video_get_fake_pwroff_logo();
		ret = video_bmp_display(vdev, map_to_sysmem(data), CONFIG_VIDEO_LOGO_X, CONFIG_VIDEO_LOGO_Y, true);
	}

	// Waiting for button press
	int fail = 0;
	while (true) {
		fail++;
		if (fail > 5) {
			printf("[FAKE_PWROFF] Too many comm failures! Breaking out of low batt loop\n");
			break;  // Stop us from getting trapped in this loop
		}

		mdelay(100);
		// Read pending IRQs and get them
		ret = dm_i2c_reg_read(axp2585_bmu, BMU_IRQ_PWRBTN_STAT);
		if (ret < 0) continue;
		int val = ret;

		// Ack pending IRQs
		ret = dm_i2c_reg_write(axp2585_bmu, BMU_IRQ_PWRBTN_STAT, val & irq_mask);

		// Process IRQs
		if (val & BMU_PWRBTN_POSEDGE_MASK) {
			if (!display_keepalive_ticks) {
				display_keepalive_ticks = display_keepalive_tick_full;
				if (blops && bldev && blops->set_brightness)
					blops->set_brightness(bldev, 100);
			}
		}
		else if (val & BMU_PWRBTN_LONG_MASK) {
			printf("[FAKE_PWROFF] Power On Requested\n");
			break;
		}

		// Handle charger removal
		ret = dm_i2c_reg_read(axp2585_bmu, BMU_CHG_STATUS);
		if (ret < 0) continue;
		if (!(ret & VBUS_GD_MASK)) {
			// VBus is gone, we're no longer charging
			printf("[FAKE_PWROFF] USB Power Removed! Shutting down device\n");
			ret = dm_i2c_reg_clrset(axp2585_bmu, BMU_BATFET_CTL,
				BMU_BATFET_OFF_MASK, BMU_BATFET_OFF_MASK);
			if (ret < 0) continue;
			mdelay(500);
			printf("[FAKE_PWROFF] Still Alive!?! Continuing in idle...\n");
			// Clear if we're still alive since if it's still stuck, it won't retrigger
			ret = dm_i2c_reg_clrset(axp2585_bmu, BMU_BATFET_CTL,
				BMU_BATFET_OFF_MASK, 0);
			if (ret < 0) continue;
		}


		// Count down battery icon rendering
		if (display_keepalive_ticks && !(--display_keepalive_ticks)) {
			if (blops && bldev && blops->set_brightness)
				blops->set_brightness(bldev, 0);
		}

		fail = 0;
	}

	if (vdev) {
		// Display the normal splash screen now that we're booting up
		video_clear(vdev);
		u8 *data = (u8*) video_get_u_boot_logo();
		ret = video_bmp_display(vdev, map_to_sysmem(data), CONFIG_VIDEO_LOGO_X, CONFIG_VIDEO_LOGO_Y, true);
	}

	if (blops && bldev && blops->set_brightness)
		blops->set_brightness(bldev, 100);

	return 0;
}
#endif

#if !CONFIG_IS_ENABLED(ARM_PSCI_FW) && !IS_ENABLED(CONFIG_SYSRESET_CMD_POWEROFF) \
	&& IS_ENABLED(CONFIG_SUNXI_NO_PMIC)
int do_poweroff(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (!axp2585_bmu) {
		axp_err("Cannot enter powerdown - no BMU probed\n");
		return -ENODEV;
	}

	int ret;
	ret = dm_i2c_reg_read(axp2585_bmu, BMU_CHG_STATUS);
	if (ret < 0) {
		axp_err("Failed to read charge status\n");
		return ret;
	}

	if (ret & VBUS_GD_MASK) {
		printf("Device plugged in, falling back to fake poweroff\n");

		// Reset requesting that we go to fake poweroff screen
		axp2585_set_fake_pwroff_requested(axp2585_bmu, 1);
		do_reset(NULL, 0, 0, NULL);

		// Shouldn't get here
		return -EINVAL;
	}

	axp_info("Turning off BATFET\n");
	ret = dm_i2c_reg_clrset(axp2585_bmu, BMU_BATFET_CTL,
				BMU_BATFET_OFF_MASK, BMU_BATFET_OFF_MASK);

	if (ret) {
		axp_err("Failed to turn off batfet: %d\n", ret);
		return ret;
	}

	/* Wait for device to shut down */
	do {} while(1);
}
#endif

static const struct udevice_id axp2585_ids[] = {
	{ .compatible = "xpower,axp2585" },
	{ }
};

U_BOOT_DRIVER(bmu_axp2585) = {
	.name = "axp2585_bmu",
	.id = UCLASS_I2C_GENERIC,
	.of_match = axp2585_ids,
	.probe = axp2585_probe,
};
