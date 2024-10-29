// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024
 * Written by Robert Pafford
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <linux/delay.h>
#include <dm/device-internal.h>

#define BITBANG_DELAY_US 2

struct pio_port_cfg_regs {
    uint32_t cfg[3];
    uint32_t pad0;
    uint32_t dat;
    uint32_t drv[4];
    uint32_t pull[2];
    uint32_t pad1;
};
static_assert(sizeof(struct pio_port_cfg_regs) == 0x30, "PIO Port config did not pack properly");

struct pio_eint_regs {
    uint32_t cfg[3];
    uint32_t pad0;
    uint32_t ctl;
    uint32_t status;
    uint32_t deb;
    uint32_t pad1;
};
static_assert(sizeof(struct pio_eint_regs) == 0x20, "PIO EINT regis did not pack properly");

struct pio_regs {
    uint32_t pad0[0xC];
    struct pio_port_cfg_regs port_cfg[6];
    uint32_t pad1[0x34];
    struct pio_eint_regs eint[6];
    uint32_t pad2[0x18];
    uint32_t pio_pow_mod_sel;
    uint32_t pio_pow_ms_ctl;
    uint32_t pio_pow_val;
    uint32_t pow_pow_vol_sel_ctl;
};

struct bringup_io_state {
	bool enable_valid;
	uint32_t enable_pin;
	uint32_t cs_pin;
	uint32_t sck_pin;
	uint32_t sda_pin;
	volatile struct pio_regs* pio_hw;
};

typedef unsigned int uint;

static void set_gpio_pin(volatile struct pio_regs* pio_hw, uint pin_desc, uint val) {
	uint port = (pin_desc >> 16) - 1;
	uint pin = pin_desc & 0xFFFF;
    if (val) {
        pio_hw->port_cfg[port].dat |= (1 << pin);
    }
    else {
        pio_hw->port_cfg[port].dat &= ~(1 << pin);
    }
}

static uint get_gpio_pin(volatile struct pio_regs* pio_hw, uint pin_desc) {
	uint port = (pin_desc >> 16) - 1;
	uint pin = pin_desc & 0xFFFF;
	return !!(pio_hw->port_cfg[port].dat & (1 << pin));
}

static uint get_gpio_pinmux(volatile struct pio_regs* pio_hw, uint pin_desc) {
	uint port = (pin_desc >> 16) - 1;
	uint pin = pin_desc & 0xFFFF;
    uint shift = (pin % 8) * 4;
    return (pio_hw->port_cfg[port].cfg[pin / 8] >> shift) & 0xF;
}

static void set_gpio_pinmux(volatile struct pio_regs* pio_hw, uint pin_desc, uint val) {
	uint port = (pin_desc >> 16) - 1;
	uint pin = pin_desc & 0xFFFF;
    uint shift = (pin % 8) * 4;
    uint keepmask = ~(0xF << shift);
    pio_hw->port_cfg[port].cfg[pin / 8] = (pio_hw->port_cfg[port].cfg[pin / 8] & keepmask) | (val << shift);
}

enum write_operation {
    Command = 0,
    Parameter = 1
};

static void SetCS(struct bringup_io_state *priv, int val) {
    set_gpio_pin(priv->pio_hw, priv->cs_pin, val);
    udelay(BITBANG_DELAY_US);
}

static void ClockBit(struct bringup_io_state *priv, int bit) {
    set_gpio_pin(priv->pio_hw, priv->sda_pin, bit);
    udelay(BITBANG_DELAY_US);
    set_gpio_pin(priv->pio_hw, priv->sck_pin, 1);
    udelay(BITBANG_DELAY_US);
    // Data kept stable here
    udelay(BITBANG_DELAY_US);
    set_gpio_pin(priv->pio_hw, priv->sck_pin, 0);
    udelay(BITBANG_DELAY_US);
}

static void Write(struct bringup_io_state *priv, enum write_operation dcx, unsigned char data) {
    SetCS(priv, 0);
    ClockBit(priv, dcx);
    for (uint i = 0; i < 8; i++) {
        ClockBit(priv, (data >> 7) & 1);
        data <<= 1;
    }
    SetCS(priv, 1);
	set_gpio_pin(priv->pio_hw, priv->sda_pin, 0);  // Clear sda when we're done
}



static int bootstrap_delay(struct udevice *dev) {
	struct bringup_io_state io_state;
	struct bringup_io_state *priv = &io_state;

	const int cell_len = 4;
	int ret;
	int size;

	size = dev_read_size(dev, "init-commands");
	if (size < 0) {
		debug("Failed to read Init Command Array: %d\n", size);
		return size;
	}
	else if (size == 0) {
		debug("Init Command Array Empty\n");
		return -EINVAL;
	}
	else if (size % cell_len != 0) {
		debug("Init Command Array has odd length: %d\n", size);
		return -EINVAL;
	}

	// Load Pins for Config
	ret = dev_read_u32(dev, "cs-pin", &priv->cs_pin);
	if (ret) {
		debug("Failed to read CS Pin: %d\n", ret);
		return ret;
	}
	ret = dev_read_u32(dev, "sck-pin", &priv->sck_pin);
	if (ret) {
		debug("Failed to read SCK Pin: %d\n", ret);
		return ret;
	}
	ret = dev_read_u32(dev, "sda-pin", &priv->sda_pin);
	if (ret) {
		debug("Failed to read SDA Pin: %d\n", ret);
		return ret;
	}
	ret = dev_read_u32(dev, "spi-enable-pin", &priv->enable_pin);
	if (ret) {
		debug("Failed to read enable pin (%d)... Disabling enable pin\n", ret);
		priv->enable_valid = false;
	}
	else {
		priv->enable_valid = true;
	}

	// Set address for PIO hardware
	priv->pio_hw = (volatile struct pio_regs*) 0x02000000;

    // Initialize GPIO Mapping to outputs
	uint cs_old_pin = get_gpio_pin(priv->pio_hw, priv->cs_pin);
    uint cs_old_pinmux = get_gpio_pinmux(priv->pio_hw, priv->cs_pin);
	uint sck_old_pin = get_gpio_pin(priv->pio_hw, priv->sck_pin);
    uint sck_old_pinmux = get_gpio_pinmux(priv->pio_hw, priv->sck_pin);
	uint sda_old_pin = get_gpio_pin(priv->pio_hw, priv->sda_pin);
    uint sda_old_pinmux = get_gpio_pinmux(priv->pio_hw, priv->sda_pin);
	if (priv->enable_valid) {
		set_gpio_pin(priv->pio_hw, priv->enable_pin, 1);
		set_gpio_pinmux(priv->pio_hw, priv->enable_pin, 1);
	}
    set_gpio_pin(priv->pio_hw, priv->cs_pin, 1);
    set_gpio_pinmux(priv->pio_hw, priv->cs_pin, 1);
    set_gpio_pinmux(priv->pio_hw, priv->sck_pin, 1);
    set_gpio_pinmux(priv->pio_hw, priv->sda_pin, 1);

    // Perform the bootstrap sequence
	if (priv->enable_valid) {
		set_gpio_pin(priv->pio_hw, priv->enable_pin, 0);
	}
	for (int i = 0; i < size / cell_len; i++) {
		unsigned int cmd;
		ret = dev_read_u32_index(dev, "init-commands", i, &cmd);
		if (ret) {
			debug("Failed to read Init Command Index %d: %d\n", i, ret);
			return ret;
		}

		if (cmd & (1 << 31)) {
			Write(priv, Command, cmd & 0xFF);
		}
		else if (cmd & (1 << 30)) {
			int delay = cmd & 0xFFFF;
			udelay(delay * 1000);
		}
		else {
			Write(priv, Parameter, cmd & 0xFF);
		}
	}
	if (priv->enable_valid) {
		set_gpio_pin(priv->pio_hw, priv->enable_pin, 1);
	}

    // Cleanup pin mapping (don't restore or gate though)
    set_gpio_pinmux(priv->pio_hw, priv->cs_pin, cs_old_pinmux);
	set_gpio_pin(priv->pio_hw, priv->cs_pin, cs_old_pin);
    set_gpio_pinmux(priv->pio_hw, priv->sck_pin, sck_old_pinmux);
	set_gpio_pin(priv->pio_hw, priv->sck_pin, sck_old_pin);
    set_gpio_pinmux(priv->pio_hw, priv->sda_pin, sda_old_pinmux);
	set_gpio_pin(priv->pio_hw, priv->sda_pin, sda_old_pin);

	return 0;
}

struct sitronix_st7701_priv {
	struct udevice *panel;
};

static int sitronix_st7701_bind(struct udevice *dev)
{
	// struct sitronix_st7701_priv *priv = dev_get_priv(dev);
	// struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	int ret;

	ret = bootstrap_delay(dev);
	if (ret) {
		return ret;
	}

	struct udevice *child_panel;
	ret = device_bind_with_driver_data(dev,
						   DM_DRIVER_GET(simple_panel),
						   "panel_core", 0,
						   dev_ofnode(dev), &child_panel);

	return 0;
}

static const struct udevice_id sitronix_st7701_ids[] = {
	{ .compatible = "sitronix,st7701" },
	{ }
};

U_BOOT_DRIVER(sitronix_st7701) = {
	.name		= "sitronix_st7701",
	.id		= UCLASS_NOP,
	.of_match	= sitronix_st7701_ids,
	// .of_to_plat	= sitronix_st7701_of_to_plat,
	.bind		= sitronix_st7701_bind,
	.priv_auto	= sizeof(struct sitronix_st7701_priv),
};
