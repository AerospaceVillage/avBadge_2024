/*
 * drivers/leds/leds-sunxi.c - Allwinner RGB LED Driver
 *
 * Copyright (C) 2018 Allwinner Technology Limited. All rights reserved.
 *      http://www.allwinnertech.com
 *
 *Author : Albert Yu <yuxyun@allwinnertech.com>
 *	   Lewis <liuyu@allwinnertech.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif
#include "leds-sunxi.h"

/* For debug */
#define LED_ERR(fmt, arg...) pr_err("sunxi-leds: %s():%d - "fmt, __func__, __LINE__, ##arg)

#define dprintk(level_mask, fmt, arg...)				\
do {									\
	if (unlikely(debug_mask & level_mask))				\
		pr_warn("%s():%d - "fmt, __func__, __LINE__, ##arg);	\
} while (0)

#ifdef DEBUG
static u32 debug_mask = DEBUG_INIT;
#else
static u32 debug_mask = 0;
#endif

struct sunxi_led *sunxi_led_global;

#define sunxi_slave_id(d, s) (((d)<<16) | (s))

static void sunxi_led_set_all(struct sunxi_led *led, u8 channel,
		enum led_brightness value);

/*For Driver */
void led_dump_reg(struct sunxi_led *led, u32 offset, u32 len)
{
	u32 i;
	u8 buf[64], cnt = 0;

	for (i = 0; i < len; i = i + REG_INTERVAL) {
		if (i%HEXADECIMAL == 0)
			cnt += sprintf(buf + cnt, "0x%08x: ",
					(u32)(led->res->start + offset + i));

		cnt += sprintf(buf + cnt, "%08x ",
				readl(led->iomem_reg_base + offset + i));

		if (i%HEXADECIMAL == REG_CL) {
			pr_warn("%s\n", buf);
			cnt = 0;
		}
	}
}

static void sunxi_clk_get(struct sunxi_led *led)
{
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	led->clk_ledc = of_clk_get(np, 0);
	if (IS_ERR(led->clk_ledc))
		LED_ERR("failed to get clk_ledc!\n");

	led->clk_cpuapb = of_clk_get(np, 1);
	if (IS_ERR(led->clk_cpuapb))
		LED_ERR("failed to get clk_cpuapb!\n");
}

static void sunxi_clk_put(struct sunxi_led *led)
{
	clk_put(led->clk_ledc);
	clk_put(led->clk_cpuapb);
	led->clk_ledc = NULL;
	led->clk_cpuapb = NULL;
}

static void sunxi_clk_enable(struct sunxi_led *led)
{
	clk_prepare_enable(led->clk_ledc);
	clk_prepare_enable(led->clk_cpuapb);
}

static void sunxi_clk_disable(struct sunxi_led *led)
{
	clk_disable_unprepare(led->clk_ledc);
}

static void sunxi_clk_init(struct sunxi_led *led)
{
	sunxi_clk_get(led);
	sunxi_clk_enable(led);
}

static void sunxi_clk_deinit(struct sunxi_led *led)
{
	sunxi_clk_disable(led);
	sunxi_clk_put(led);
}

static u32 sunxi_get_reg(int offset)
{
	struct sunxi_led *led = sunxi_led_global;
	u32 value = ioread32(((u8 *)led->iomem_reg_base) + offset);

	return value;
}

static void sunxi_set_reg(int offset, u32 value)
{
	struct sunxi_led *led = sunxi_led_global;

	iowrite32(value, ((u8 *)led->iomem_reg_base) + offset);
}

static inline void sunxi_set_reset_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x1FFF;
	u32 min = SUNXI_RESET_TIME_MIN_NS;
	u32 max = SUNXI_RESET_TIME_MAX_NS;

	if (led->reset_ns < min || led->reset_ns > max) {
		LED_ERR("invalid parameter, reset_ns should be %u-%u!\n",
				min, max);
		return;
	}

	n = (led->reset_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_RESET_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << 16);
	reg_val |= (n << 16);
	sunxi_set_reg(LED_RESET_TIMING_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_set_t1h_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x3F;
	u32 shift = 21;
	u32 min = SUNXI_T1H_MIN_NS;
	u32 max = SUNXI_T1H_MAX_NS;

	if (led->t1h_ns < min || led->t1h_ns > max) {
		LED_ERR("invalid parameter, t1h_ns should be %u-%u!\n",
				min, max);
		return;
	}

	n = (led->t1h_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << shift);
	reg_val |= n << shift;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_set_t1l_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x1F;
	u32 shift = 16;
	u32 min = SUNXI_T1L_MIN_NS;
	u32 max = SUNXI_T1L_MAX_NS;

	if (led->t1l_ns < min || led->t1l_ns > max) {
		LED_ERR("invalid parameter, t1l_ns should be %u-%u!\n",
				min, max);
		return;
	}

	n = (led->t1l_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << shift);
	reg_val |= n << shift;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_set_t0h_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 mask = 0x1F;
	u32 shift = 6;
	u32 min = SUNXI_T0H_MIN_NS;
	u32 max = SUNXI_T0H_MAX_NS;

	if (led->t0h_ns < min || led->t0h_ns > max) {
		LED_ERR("invalid parameter, t0h_ns should be %u-%u!\n",
			min, max);
		return;
	}

	n = (led->t0h_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~(mask << shift);
	reg_val |= n << shift;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_set_t0l_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_T0L_MIN_NS;
	u32 max = SUNXI_T0L_MAX_NS;

	if (led->t0l_ns < min || led->t0l_ns > max) {
		LED_ERR("invalid parameter, t0l_ns should be %u-%u!\n",
				min, max);
		return;
	}

	n = (led->t0l_ns - 42) / 42;
	reg_val = sunxi_get_reg(LED_T01_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~0x3F;
	reg_val |= n;
	sunxi_set_reg(LED_T01_TIMING_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_set_wait_time0_ns(struct sunxi_led *led)
{
	u32 n, reg_val;
	u32 min = SUNXI_WAIT_TIME0_MIN_NS;
	u32 max = SUNXI_WAIT_TIME0_MAX_NS;

	if (led->wait_time0_ns < min || led->wait_time0_ns > max) {
		LED_ERR("invalid parameter, wait_time0_ns should be %u-%u!\n",
				min, max);
		return;
	}

	n = (led->wait_time0_ns - 42) / 42;
	reg_val = (1 << 8) | n;
	sunxi_set_reg(LEDC_WAIT_TIME0_CTRL_REG, reg_val);
}

static inline void sunxi_set_wait_time1_ns(struct sunxi_led *led)
{
	unsigned long long tmp, max = SUNXI_WAIT_TIME1_MAX_NS;
	u32 min = SUNXI_WAIT_TIME1_MIN_NS;
	u32 n, reg_val;

	if (led->wait_time1_ns < min || led->wait_time1_ns > max) {
		LED_ERR("invalid parameter, wait_time1_ns should be %u-%llu!\n",
			min, max);
		return;
	}

	tmp = led->wait_time1_ns;
	n = div_u64(tmp, 42);
	n -= 1;
	reg_val = (1 << 31) | n;
	sunxi_set_reg(LEDC_WAIT_TIME1_CTRL_REG, reg_val);
}

static inline void sunxi_set_wait_data_time_ns(struct sunxi_led *led)
{
	u32 min, max;
#ifndef SUNXI_FPGA_LEDC
	u32 mask = 0x1FFF, shift = 16, reg_val = 0, n;
#endif
	min = SUNXI_WAIT_DATA_TIME_MIN_NS;
#ifdef SUNXI_FPGA_LEDC
	/*
	 * For FPGA platforms, it is easy to meet wait data timeout for
	 * the obvious latency of task which is because of less cpu cores
	 * and lower cpu frequency compared with IC platforms, so here we
	 * permit long enough time latency.
	 */
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_FPGA;
#else /* SUNXI_FPGA_LEDC */
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_IC;
#endif /* SUNXI_FPGA_LEDC */

	if (led->wait_data_time_ns < min || led->wait_data_time_ns > max) {
		LED_ERR("invalid parameter, wait_data_time_ns should be %u-%u!\n",
			min, max);
		return;
	}

#ifndef SUNXI_FPGA_LEDC
	n = (led->wait_data_time_ns - 42) / 42;
	reg_val &= ~(mask << shift);
	reg_val |= (n << shift);
	sunxi_set_reg(LEDC_DATA_FINISH_CNT_REG_OFFSET, reg_val);
#endif /* SUNXI_FPGA_LEDC */
}

static void sunxi_ledc_set_time(struct sunxi_led *led)
{
	sunxi_set_reset_ns(led);
	sunxi_set_t1h_ns(led);
	sunxi_set_t1l_ns(led);
	sunxi_set_t0h_ns(led);
	sunxi_set_t0l_ns(led);
	sunxi_set_wait_time0_ns(led);
	sunxi_set_wait_time1_ns(led);
	sunxi_set_wait_data_time_ns(led);
}

static void sunxi_ledc_set_length(struct sunxi_led *led)
{
	u32 reg_val;
	u32 length = led->length;

	if (length == 0)
		return;

	if (length > led->led_count)
		return;

	reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);
	reg_val &= ~(0x1FFF << 16);
	reg_val |=  length << 16;
	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);

	reg_val = sunxi_get_reg(LED_RESET_TIMING_CTRL_REG_OFFSET);
	reg_val &= ~0x3FF;
	reg_val |= length - 1;
	sunxi_set_reg(LED_RESET_TIMING_CTRL_REG_OFFSET, reg_val);
}

static void sunxi_ledc_set_output_mode(struct sunxi_led *led, const char *str)
{
	u32 val;
	u32 mask = 0x7;
	u32 shift = 6;
	u32 reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);
	if (str != NULL) {
		if (!strncmp(str, "GRB", 3))
			val = SUNXI_OUTPUT_GRB;
		else if (!strncmp(str, "GBR", 3))
			val = SUNXI_OUTPUT_GBR;
		else if (!strncmp(str, "RGB", 3))
			val = SUNXI_OUTPUT_RGB;
		else if (!strncmp(str, "RBG", 3))
			val = SUNXI_OUTPUT_RBG;
		else if (!strncmp(str, "BGR", 3))
			val = SUNXI_OUTPUT_BGR;
		else if (!strncmp(str, "BRG", 3))
			val = SUNXI_OUTPUT_BRG;
		else
			return;
	} else {
		val = led->output_mode.val;
	}

	reg_val &= ~(mask << shift);
	reg_val |= val;

	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);

	if (strncmp(str, led->output_mode.str, 3))
		memcpy(led->output_mode.str, str, 3);

	if (val != led->output_mode.val)
		led->output_mode.val = val;
}

static void sunxi_ledc_enable_irq(u32 mask)
{
	u32 reg_val = 0;

	reg_val |= mask;
	sunxi_set_reg(LEDC_INT_CTRL_REG_OFFSET, reg_val);
}

static void sunxi_ledc_disable_irq(u32 mask)
{
	u32 reg_val = 0;

	reg_val = sunxi_get_reg(LEDC_INT_CTRL_REG_OFFSET);
	reg_val &= ~mask;
	sunxi_set_reg(LEDC_INT_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_ledc_enable(struct sunxi_led *led)
{
	u32 reg_val;

	reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);
	reg_val |=  1;
	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);
}

static inline void sunxi_ledc_reset(struct sunxi_led *led)
{
	u32 reg_val = sunxi_get_reg(LEDC_CTRL_REG_OFFSET);

	sunxi_ledc_disable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_FIFO_CPUREQ_INT_EN
			| LEDC_WAITDATA_TIMEOUT_INT_EN | LEDC_FIFO_OVERFLOW_INT_EN
			| LEDC_GLOBAL_INT_EN);

	if (debug_mask & DEBUG_INFO2) {
		dprintk(DEBUG_INFO2, "dump reg:\n");
		led_dump_reg(led, 0, 0x30);
	}

	reg_val |= 1 << 1;
	sunxi_set_reg(LEDC_CTRL_REG_OFFSET, reg_val);
}

#ifdef CONFIG_DEBUG_FS
static ssize_t reset_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_RESET_TIME_MIN_NS;
	max = SUNXI_RESET_TIME_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->reset_ns = val;
	sunxi_set_reset_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, reset_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t reset_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->reset_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations reset_ns_fops = {
	.owner = THIS_MODULE,
	.write = reset_ns_write,
	.read  = reset_ns_read,
};

static ssize_t t1h_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_T1H_MIN_NS;
	max = SUNXI_T1H_MAX_NS;

	if (count >= sizeof(buffer))
		return -EINVAL;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		return -EINVAL;

	if (val < min || val > max)
		goto err_out;

	led->t1h_ns = val;

	sunxi_set_t1h_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, t1h_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t1h_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->t1h_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t1h_ns_fops = {
	.owner = THIS_MODULE,
	.write = t1h_ns_write,
	.read  = t1h_ns_read,
};

static ssize_t t1l_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_T1L_MIN_NS;
	max = SUNXI_T1L_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t1l_ns = val;
	sunxi_set_t1l_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, t1l_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t1l_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->t1l_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t1l_ns_fops = {
	.owner = THIS_MODULE,
	.write = t1l_ns_write,
	.read  = t1l_ns_read,
};

static ssize_t t0h_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_T0H_MIN_NS;
	max = SUNXI_T0H_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t0h_ns = val;
	sunxi_set_t0h_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, t0h_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t0h_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->t0h_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t0h_ns_fops = {
	.owner = THIS_MODULE,
	.write = t0h_ns_write,
	.read  = t0h_ns_read,
};

static ssize_t t0l_ns_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_T0L_MIN_NS;
	max = SUNXI_T0L_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->t0l_ns = val;
	sunxi_set_t0l_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, t0l_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t t0l_ns_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->t0l_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations t0l_ns_fops = {
	.owner = THIS_MODULE,
	.write = t0l_ns_write,
	.read  = t0l_ns_read,
};

static ssize_t wait_time0_ns_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_WAIT_TIME0_MIN_NS;
	max = SUNXI_WAIT_TIME0_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_time0_ns = val;
	sunxi_set_wait_time0_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, wait_time0_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t wait_time0_ns_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->wait_time0_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations wait_time0_ns_fops = {
	.owner = THIS_MODULE,
	.write = wait_time0_ns_write,
	.read  = wait_time0_ns_read,
};

static ssize_t wait_time1_ns_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min;
	unsigned long long max;
	unsigned long long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_WAIT_TIME1_MIN_NS;
	max = SUNXI_WAIT_TIME1_MAX_NS;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoull(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_time1_ns = val;
	sunxi_set_wait_time1_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, wait_time1_ns should be %u-%lld!\n",
		min, max);

	return -EINVAL;
}

static ssize_t wait_time1_ns_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%lld\n", led->wait_time1_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations wait_time1_ns_fops = {
	.owner = THIS_MODULE,
	.write = wait_time1_ns_write,
	.read  = wait_time1_ns_read,
};

static ssize_t wait_data_time_ns_write(struct file *filp,
				const char __user *buf,
				size_t count, loff_t *offp)
{
	int err;
	char buffer[64];
	u32 min, max;
	unsigned long val;
	struct sunxi_led *led = sunxi_led_global;

	min = SUNXI_WAIT_DATA_TIME_MIN_NS;
#ifdef SUNXI_FPGA_LEDC
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_FPGA;
#else
	max = SUNXI_WAIT_DATA_TIME_MAX_NS_IC;
#endif

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	err = kstrtoul(buffer, 10, &val);
	if (err)
		goto err_out;

	if (val < min || val > max)
		goto err_out;

	led->wait_data_time_ns = val;
	sunxi_set_wait_data_time_ns(led);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter, wait_data_time_ns should be %u-%u!\n",
		min, max);

	return -EINVAL;
}

static ssize_t wait_data_time_ns_read(struct file *filp, char __user *buf,
				size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%u\n", led->wait_data_time_ns);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations wait_data_time_ns_fops = {
	.owner = THIS_MODULE,
	.write = wait_data_time_ns_write,
	.read  = wait_data_time_ns_read,
};

static int data_show(struct seq_file *s, void *data)
{
	int i;
	struct sunxi_led *led = sunxi_led_global;

	for (i = 0; i < led->led_count; i++) {
		if (!(i % 4)) {
			if (i + 4 <= led->led_count)
				seq_printf(s, "%04d-%04d", i, i + 4);
			else
				seq_printf(s, "%04d-%04d", i, led->led_count);
		}
		seq_printf(s, " 0x%08x", led->data[i]);
		if (((i % 4) == 3) || (i == led->led_count - 1))
			seq_puts(s, "\n");
	}

	return 0;
}

static void sunxi_ledc_set_dma_mode(struct sunxi_led *led)
{
	u32 reg_val = 0;

	reg_val |= 1 << 5;
	sunxi_set_reg(LEDC_DMA_CTRL_REG, reg_val);

	sunxi_ledc_disable_irq(LEDC_FIFO_CPUREQ_INT_EN);
}

static void sunxi_ledc_set_cpu_mode(struct sunxi_led *led)
{
	u32 reg_val = 0;

	reg_val &= ~(1 << 5);
	sunxi_set_reg(LEDC_DMA_CTRL_REG, reg_val);

	sunxi_ledc_enable_irq(LEDC_FIFO_CPUREQ_INT_EN);
}

static int data_open(struct inode *inode, struct file *file)
{
	return single_open(file, data_show, inode->i_private);
}

static const struct file_operations data_fops = {
	.owner = THIS_MODULE,
	.open  = data_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t output_mode_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *offp)
{
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		goto err_out;

	buffer[count] = '\0';

	sunxi_ledc_set_output_mode(led, buffer);

	*offp += count;

	return count;

err_out:
	LED_ERR("invalid parameter!\n");

	return -EINVAL;
}

static ssize_t output_mode_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	struct sunxi_led *led = sunxi_led_global;

	r = snprintf(buffer, 64, "%s\n", led->output_mode.str);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations output_mode_fops = {
	.owner = THIS_MODULE,
	.write = output_mode_write,
	.read  = output_mode_read,
};

static ssize_t hwversion_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	int r;
	char buffer[64];
	u32 reg_val, major_ver, minor_ver;

	reg_val = sunxi_get_reg(LEDC_VER_NUM_REG);
	major_ver = reg_val >> 16;
	minor_ver = reg_val & 0xF;

	r = snprintf(buffer, 64, "r%up%u\n", major_ver, minor_ver);

	return simple_read_from_buffer(buf, count, offp, buffer, r);
}

static const struct file_operations hwversion_fops = {
	.owner = THIS_MODULE,
	.read  = hwversion_read,
};

static ssize_t ledtest_read(struct file *filp, char __user *buf,
			size_t count, loff_t *offp)
{
	struct sunxi_led *led = sunxi_led_global;

	sunxi_led_set_all(led, 0, 0);
	sunxi_led_set_all(led, 1, 0);
	sunxi_led_set_all(led, 2, 0);

	sunxi_led_set_all(led, 0, 20);
	msleep(500);
	sunxi_led_set_all(led, 1, 20);
	msleep(500);
	sunxi_led_set_all(led, 2, 20);
	msleep(500);

	sunxi_led_set_all(led, 0, 0);
	sunxi_led_set_all(led, 1, 0);
	sunxi_led_set_all(led, 2, 0);

	return 0;
}

static const struct file_operations ledtest_fops = {
	.owner = THIS_MODULE,
	.read  = ledtest_read,
};

static void sunxi_led_create_debugfs(struct sunxi_led *led)
{
	struct dentry *debugfs_dir, *debugfs_file;

	debugfs_dir = debugfs_create_dir("sunxi_leds", NULL);
	if (IS_ERR_OR_NULL(debugfs_dir)) {
		LED_ERR("debugfs_create_dir failed!\n");
		return;
	}

	led->debugfs_dir = debugfs_dir;

	debugfs_file = debugfs_create_file("reset_ns", 0660,
				debugfs_dir, NULL, &reset_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for reset_ns failed!\n");

	debugfs_file = debugfs_create_file("t1h_ns", 0660,
				debugfs_dir, NULL, &t1h_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for t1h_ns failed!\n");

	debugfs_file = debugfs_create_file("t1l_ns", 0660,
				debugfs_dir, NULL, &t1l_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for t1l_ns failed!\n");

	debugfs_file = debugfs_create_file("t0h_ns", 0660,
				debugfs_dir, NULL, &t0h_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for t0h_ns failed!\n");

	debugfs_file = debugfs_create_file("t0l_ns", 0660,
				debugfs_dir, NULL, &t0l_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for t0l_ns failed!\n");

	debugfs_file = debugfs_create_file("wait_time0_ns", 0660,
				debugfs_dir, NULL, &wait_time0_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for wait_time0_ns failed!\n");

	debugfs_file = debugfs_create_file("wait_time1_ns", 0660,
				debugfs_dir, NULL, &wait_time1_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for wait_time1_ns failed!\n");

	debugfs_file = debugfs_create_file("wait_data_time_ns", 0660,
				debugfs_dir, NULL, &wait_data_time_ns_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for wait_data_time_ns failed!\n");

	debugfs_file = debugfs_create_file("data", 0440,
				debugfs_dir, NULL, &data_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for data failed!\n");

	debugfs_file = debugfs_create_file("output_mode", 0660,
				debugfs_dir, NULL, &output_mode_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for output_mode failed!\n");

	if (!debugfs_file)
		LED_ERR("debugfs_create_file for trans_mode failed!\n");

	debugfs_file = debugfs_create_file("hwversion", 0440,
				debugfs_dir, NULL, &hwversion_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for hwversion failed!\n");

	debugfs_file = debugfs_create_file("ledtest", 0440,
				debugfs_dir, NULL, &ledtest_fops);
	if (!debugfs_file)
		LED_ERR("debugfs_create_file for ledtest failed!\n");
}

static void sunxi_led_remove_debugfs(struct sunxi_led *led)
{
	debugfs_remove_recursive(led->debugfs_dir);
}
#endif /* CONFIG_DEBUG_FS */

static void sunxi_ledc_dma_callback(void *param)
{
	dprintk(DEBUG_INFO, "finish\n");
}

static void sunxi_ledc_trans_data(struct sunxi_led *led)
{
	int i, err;
	size_t size;
	unsigned long flags;
	phys_addr_t dst_addr;
	struct dma_slave_config slave_config;
	struct device *dev = led->dev;
	struct dma_async_tx_descriptor *dma_desc;

	/* less than 32 lights use cpu transmission. */
	/* more than 32 lights use dma transmission. */
	if (led->length <= SUNXI_LEDC_FIFO_DEPTH) {
		dprintk(DEBUG_INFO, "cpu xfer\n");
		ktime_get_coarse_real_ts64(&(led->start_time));
		sunxi_ledc_set_time(led);
		sunxi_ledc_set_output_mode(led, led->output_mode.str);
		sunxi_ledc_set_cpu_mode(led);
		sunxi_ledc_set_length(led);

		sunxi_ledc_enable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_WAITDATA_TIMEOUT_INT_EN
				| LEDC_FIFO_OVERFLOW_INT_EN | LEDC_GLOBAL_INT_EN);

		sunxi_ledc_enable(led);

		for (i = 0; i < led->length; i++)
			sunxi_set_reg(LEDC_DATA_REG_OFFSET, led->data[i]);

	} else {
		dprintk(DEBUG_INFO, "dma xfer\n");

		size = led->length * 4;
		led->src_dma = dma_map_single(dev, led->data,
					size, DMA_TO_DEVICE);
		dst_addr = led->res->start + LEDC_DATA_REG_OFFSET;

		flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK;

		slave_config.direction = DMA_MEM_TO_DEV;
		slave_config.src_addr = led->src_dma;
		slave_config.dst_addr = dst_addr;
		slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		slave_config.src_maxburst = 4;
		slave_config.dst_maxburst = 4;

		err = dmaengine_slave_config(led->dma_chan, &slave_config);
		if (err < 0) {
			LED_ERR("dmaengine_slave_config failed!\n");
			return;
		}

		dma_desc = dmaengine_prep_slave_single(led->dma_chan,
							led->src_dma,
							size,
							DMA_MEM_TO_DEV,
							flags);
		if (!dma_desc) {
			LED_ERR("dmaengine_prep_slave_single failed!\n");
			return;
		}

		dma_desc->callback = sunxi_ledc_dma_callback;

		dmaengine_submit(dma_desc);
		dma_async_issue_pending(led->dma_chan);

		ktime_get_coarse_real_ts64(&(led->start_time));
		sunxi_ledc_set_time(led);
		sunxi_ledc_set_output_mode(led, led->output_mode.str);
		sunxi_ledc_set_dma_mode(led);
		sunxi_ledc_set_length(led);
		sunxi_ledc_enable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_WAITDATA_TIMEOUT_INT_EN
				| LEDC_FIFO_OVERFLOW_INT_EN | LEDC_GLOBAL_INT_EN);
		sunxi_ledc_enable(led);
	}
}

static inline void sunxi_ledc_clear_all_irq(void)
{
	u32 reg_val = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);

	reg_val &= ~0x1F;
	sunxi_set_reg(LEDC_INT_STS_REG_OFFSET, reg_val);
}

static inline void sunxi_ledc_clear_irq(enum sunxi_ledc_irq_status_reg irq)
{
	u32 reg_val = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);

	reg_val &= ~irq;
	sunxi_set_reg(LEDC_INT_STS_REG_OFFSET, reg_val);
}

static void sunxi_ledc_dma_terminate(struct sunxi_led *led)
{
	if (led->dma_chan) {
		dmaengine_terminate_all(led->dma_chan);
		dma_unmap_single(led->dev, led->src_dma, led->length * 4,
				DMA_TO_DEVICE);
	}
}

static int sunxi_ledc_complete(struct sunxi_led *led)
{
	unsigned long flags = 0;
	unsigned long timeout = 0;
	u32 reg_val;

	/*wait_event_timeout return 0   : timeout
	 *wait_event_timeout return > 0 : thr left time
	 * */
	timeout = wait_event_timeout(led->wait, led->result, 5*HZ);

	/* dynamic close dma transmission */
	sunxi_ledc_dma_terminate(led);

	if (timeout == 0) {
		reg_val = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);
		printk("LEDC INTERRUPT STATUS REG IS %x", reg_val);
		LED_ERR("led xfer timeout\n");
		reg_val = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);
		printk("LEDC INTERRUPT STATUS REG IS %x", reg_val);
		return -ETIME;
	} else if (led->result == RESULT_ERR) {
		return -ECOMM;
	}

	dprintk(DEBUG_INFO, "xfer complete\n");

	spin_lock_irqsave(&led->lock, flags);
	led->result = 0;
	spin_unlock_irqrestore(&led->lock, flags);

	return 0;
}

static irqreturn_t sunxi_ledc_irq_handler(int irq, void *dev_id)
{
	unsigned long flags;
	long delta_time_ns;
	u32 irq_status, max_ns;
	struct sunxi_led *led = sunxi_led_global;
	struct timespec64 current_time;

	spin_lock_irqsave(&led->lock, flags);

	irq_status = sunxi_get_reg(LEDC_INT_STS_REG_OFFSET);

	sunxi_ledc_clear_all_irq();

	if (irq_status & LEDC_TRANS_FINISH_INT) {
		sunxi_ledc_reset(led);
		led->length = 0;
		led->result = RESULT_COMPLETE;
		wake_up(&led->wait);
		goto out;
	}

	if (irq_status & LEDC_WAITDATA_TIMEOUT_INT) {
		ktime_get_coarse_real_ts64(&current_time);
		delta_time_ns = current_time.tv_sec - led->start_time.tv_sec;
		delta_time_ns *= 1000 * 1000 * 1000;
		delta_time_ns += current_time.tv_nsec - led->start_time.tv_nsec;

		max_ns = led->wait_data_time_ns;

		if (delta_time_ns <= max_ns) {
			spin_unlock_irqrestore(&led->lock, flags);
			return IRQ_HANDLED;
		}

		sunxi_ledc_reset(led);

		if (delta_time_ns <= max_ns * 2) {
			sunxi_ledc_dma_terminate(led);
			sunxi_ledc_trans_data(led);
		} else {
			LED_ERR("wait time is more than %d ns,"
				"going to reset ledc and drop this operation!\n",
				max_ns);
			led->result = RESULT_ERR;
			wake_up(&led->wait);
			led->length = 0;
		}

		goto out;
	}

	if (irq_status & LEDC_FIFO_OVERFLOW_INT) {
		LED_ERR("there exists fifo overflow issue, irq_status=0x%x!\n",
				irq_status);
		sunxi_ledc_reset(led);
		led->result = RESULT_ERR;
		wake_up(&led->wait);
		led->length = 0;
		goto out;
	}

out:
	spin_unlock_irqrestore(&led->lock, flags);
	return IRQ_HANDLED;
}

static int sunxi_ledc_irq_init(struct sunxi_led *led)
{
	int err;
	struct device *dev = led->dev;
	unsigned long flags = 0;
	const char *name = "ledcirq";
	struct platform_device *pdev;

	pdev = container_of(dev, struct platform_device, dev);

	spin_lock_init(&led->lock);

	led->irqnum = platform_get_irq(pdev, 0);
	if (led->irqnum < 0) {
		LED_ERR("failed to get ledc irq: %d\n", led->irqnum);
		return -EINVAL;
	}

	err = request_irq(led->irqnum, sunxi_ledc_irq_handler,
				flags, name, dev);
	if (err) {
		LED_ERR("failed to install IRQ handler for irqnum %d\n",
			led->irqnum);
		return -EPERM;
	}

	return 0;
}

static void sunxi_ledc_irq_deinit(struct sunxi_led *led)
{
	free_irq(led->irqnum, led->dev);
	sunxi_ledc_disable_irq(LEDC_TRANS_FINISH_INT_EN | LEDC_FIFO_CPUREQ_INT_EN
			| LEDC_WAITDATA_TIMEOUT_INT_EN | LEDC_FIFO_OVERFLOW_INT_EN
			| LEDC_GLOBAL_INT_EN);
}

static void sunxi_ledc_pinctrl_init(struct sunxi_led *led)
{
	struct device *dev = led->dev;
	struct pinctrl *pinctrl = devm_pinctrl_get_select_default(dev);

	led->pctrl = pinctrl;
	if (IS_ERR(pinctrl))
		LED_ERR("devm_pinctrl_get_select_default failed!\n");
}

static int led_regulator_request(struct sunxi_led *led)
{
	struct regulator *regu = NULL;

	/* Consider "n*" as nocare. Support "none", "nocare", "null", "" etc. */
	if ((led->regulator_id[0] == 'n') || (led->regulator_id[0] == 0))
		return 0;

	regu = regulator_get(NULL, led->regulator_id);
	if (IS_ERR(regu)) {
		LED_ERR("get regulator %s failed!\n", led->regulator_id);
		return -1;
	}
	led->regulator = regu;

	return 0;
}

static int led_regulator_release(struct sunxi_led *led)
{
	if (led->regulator == NULL)
		return 0;

	regulator_put(led->regulator);
	led->regulator = NULL;

	return 1;
}

static int sunxi_ledc_dma_get(struct sunxi_led *led)
{
	if (led->dma_chan == NULL) {
		led->dma_chan = dma_request_chan(led->dev, "tx");
		if (IS_ERR(led->dma_chan)) {
			LED_ERR("failed to get the DMA channel!\n");
			return -EFAULT;
		}
	}
	return 0;
}

static void sunxi_ledc_dma_put(struct sunxi_led *led)
{
	if (led->dma_chan) {
		dma_release_channel(led->dma_chan);
		led->dma_chan = NULL;
	}
}

static int sunxi_set_led_brightness(struct led_classdev *led_cdev,
			enum led_brightness value)
{
	unsigned long flags;
	u32 r, g, b, shift, old_data, new_data, length;
	struct sunxi_led_info *pinfo;
	struct sunxi_led_classdev_group *pcdev_group;
	struct sunxi_led *led = sunxi_led_global;
	int err;

	pinfo = container_of(led_cdev, struct sunxi_led_info, cdev);

	switch (pinfo->type) {
	case LED_TYPE_G:
		pcdev_group = container_of(pinfo,
			struct sunxi_led_classdev_group, g);
		g = value;
		shift = 16;
		break;
	case LED_TYPE_R:
		pcdev_group = container_of(pinfo,
			struct sunxi_led_classdev_group, r);
		r = value;
		shift = 8;
		break;

	case LED_TYPE_B:
		pcdev_group = container_of(pinfo,
			struct sunxi_led_classdev_group, b);
		b = value;
		shift = 0;
		break;
	}

	old_data = led->data[pcdev_group->led_num];
	if (((old_data >> shift) & 0xFF) == value)
		return 0;

	if (pinfo->type != LED_TYPE_R)
		r = pcdev_group->r.cdev.brightness;
	if (pinfo->type != LED_TYPE_G)
		g = pcdev_group->g.cdev.brightness;
	if (pinfo->type != LED_TYPE_B)
		b = pcdev_group->b.cdev.brightness;

	/* LEDC treats input data as GRB by default */
	new_data = (g << 16) | (r << 8) | b;
	length = pcdev_group->led_num + 1;

	spin_lock_irqsave(&led->lock, flags);
	led->data[pcdev_group->led_num] = new_data;
	led->length = length;
	spin_unlock_irqrestore(&led->lock, flags);

	/* prepare for dma xfer, dynamic apply dma channel */
	if (led->length > SUNXI_LEDC_FIFO_DEPTH) {
		err = sunxi_ledc_dma_get(led);
		if (err)
			return err;
	}

	sunxi_ledc_trans_data(led);
	if (debug_mask & DEBUG_INFO2) {
		dprintk(DEBUG_INFO2, "dump reg:\n");
		led_dump_reg(led, 0, 0x30);
	}

	sunxi_ledc_complete(led);

	/* dynamic release dma chan, release at the end of a transmission */
	if (led->length > SUNXI_LEDC_FIFO_DEPTH)
		sunxi_ledc_dma_put(led);

	if (debug_mask & DEBUG_INFO1)
		pr_warn("num = %03u\n", length);

	return 0;
}

static int sunxi_register_led_classdev(struct sunxi_led *led)
{
	int i, err;
	size_t size;
	struct device *dev = led->dev;
	struct led_classdev *pcdev_RGB;

	dprintk(DEBUG_INIT, "led_classdev start\n");
	if (!led->led_count)
		led->led_count = SUNXI_DEFAULT_LED_COUNT;

	size = sizeof(struct sunxi_led_classdev_group) * led->led_count;
	led->pcdev_group = kzalloc(size, GFP_KERNEL);
	if (!led->pcdev_group) {
		LED_ERR("kzalloc error\n");
		return -ENOMEM;
	}

	for (i = 0; i < led->led_count; i++) {
		led->pcdev_group[i].r.type = LED_TYPE_R;
		pcdev_RGB = &led->pcdev_group[i].r.cdev;
		pcdev_RGB->name = kzalloc(16, GFP_KERNEL);
		sprintf((char *)pcdev_RGB->name, "sunxi_led%dr", i);
		pcdev_RGB->brightness = LED_OFF;
		pcdev_RGB->brightness_set_blocking = sunxi_set_led_brightness;
		pcdev_RGB->dev = dev;
		err = led_classdev_register(dev, pcdev_RGB);
		if (err < 0) {
			LED_ERR("led_classdev_register %s failed!\n",
				pcdev_RGB->name);
			return err;
		}

		led->pcdev_group[i].g.type = LED_TYPE_G;
		pcdev_RGB = &led->pcdev_group[i].g.cdev;
		pcdev_RGB->name = kzalloc(16, GFP_KERNEL);
		sprintf((char *)pcdev_RGB->name, "sunxi_led%dg", i);
		pcdev_RGB->brightness = LED_OFF;
		pcdev_RGB->brightness_set_blocking = sunxi_set_led_brightness;
		pcdev_RGB->dev = dev;
		err = led_classdev_register(dev, pcdev_RGB);
		if (err < 0) {
			LED_ERR("led_classdev_register %s failed!\n",
			pcdev_RGB->name);
			return err;
		}

		led->pcdev_group[i].b.type = LED_TYPE_B;
		pcdev_RGB = &led->pcdev_group[i].b.cdev;
		pcdev_RGB->name = kzalloc(16, GFP_KERNEL);
		sprintf((char *)pcdev_RGB->name, "sunxi_led%db", i);
		pcdev_RGB->brightness = LED_OFF;
		pcdev_RGB->brightness_set_blocking = sunxi_set_led_brightness;
		pcdev_RGB->dev = dev;
		err = led_classdev_register(dev, pcdev_RGB);
		if (err < 0) {
			LED_ERR("led_classdev_register %s failed!\n",
					pcdev_RGB->name);
			return err;
		}

		led->pcdev_group[i].led_num = i;
	}

	size = sizeof(u32) * led->led_count;
	led->data = kzalloc(size, GFP_KERNEL);
	if (!led->data)
		return -ENOMEM;

	return 0;
}

static void sunxi_unregister_led_classdev(struct sunxi_led *led)
{
	int i;

	for (i = 0; i < led->led_count; i++) {
		kfree(led->pcdev_group[i].b.cdev.name);
		led->pcdev_group[i].b.cdev.name = NULL;
		kfree(led->pcdev_group[i].g.cdev.name);
		led->pcdev_group[i].g.cdev.name = NULL;
		kfree(led->pcdev_group[i].r.cdev.name);
		led->pcdev_group[i].r.cdev.name = NULL;
		led_classdev_unregister(&led->pcdev_group[i].b.cdev);
		led_classdev_unregister(&led->pcdev_group[i].g.cdev);
		led_classdev_unregister(&led->pcdev_group[i].r.cdev);
	}
	kfree(led->data);
	led->data = NULL;


	kfree(led->pcdev_group);
	led->pcdev_group = NULL;
}

static inline int sunxi_get_u32_of_property(const char *propname, int *val)
{
	int err;
	struct sunxi_led *led = sunxi_led_global;
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	err = of_property_read_u32(np, propname, val);
	if (err < 0)
		LED_ERR("failed to get the value of propname %s!\n", propname);

	return err;
}

static inline int sunxi_get_str_of_property(const char *propname,
					const char **out_string)
{
	int err;
	struct sunxi_led *led = sunxi_led_global;
	struct device *dev = led->dev;
	struct device_node *np = dev->of_node;

	err = of_property_read_string(np, propname, out_string);
	if (err < 0)
		LED_ERR("failed to get the string of propname %s!\n", propname);

	return err;
}

static void sunxi_get_para_of_property(struct sunxi_led *led)
{
	int err;
	u32 val;
	const char *str;

	err = sunxi_get_u32_of_property("led_count", &val);
	if (!err)
		led->led_count = val;

	memcpy(led->output_mode.str, "GRB", 3);
	led->output_mode.val = SUNXI_OUTPUT_GRB;
	err = sunxi_get_str_of_property("output_mode", &str);
	if (!err)
		if (!strncmp(str, "BRG", 3) ||
			!strncmp(str, "GBR", 3) ||
			!strncmp(str, "RGB", 3) ||
			!strncmp(str, "RBG", 3) ||
			!strncmp(str, "BGR", 3) ||
			!strncmp(str, "BRG", 3))
			memcpy(led->output_mode.str, str, 3);

	err =  sunxi_get_str_of_property("led_regulator", &str);
	if (!err) {
		if (strlen(str) >= sizeof(led->regulator_id))
			LED_ERR("illegal regulator id\n");
		else {
			strcpy(led->regulator_id, str);
			dprintk(DEBUG_INIT, "led_regulator: %s\n", led->regulator_id);
		}
	}

	err = sunxi_get_u32_of_property("reset_ns", &val);
	if (!err)
		led->reset_ns = val;

	err = sunxi_get_u32_of_property("t1h_ns", &val);
	if (!err)
		led->t1h_ns = val;

	err = sunxi_get_u32_of_property("t1l_ns", &val);
	if (!err)
		led->t1l_ns = val;

	err = sunxi_get_u32_of_property("t0h_ns", &val);
	if (!err)
		led->t0h_ns = val;

	err = sunxi_get_u32_of_property("t0l_ns", &val);
	if (!err)
		led->t0l_ns = val;

	err = sunxi_get_u32_of_property("wait_time0_ns", &val);
	if (!err)
		led->wait_time0_ns = val;

	err = sunxi_get_u32_of_property("wait_time1_ns", &val);
	if (!err)
		led->wait_time1_ns = val;

	err = sunxi_get_u32_of_property("wait_data_time_ns", &val);
	if (!err)
		led->wait_data_time_ns = val;
}

static void sunxi_led_set_all(struct sunxi_led *led, u8 channel,
		enum led_brightness value)
{
	u32 i;
	struct led_classdev *led_cdev;

	if (channel%3 == 0) {
		for (i = 0; i < led->led_count; i++) {
			led_cdev = &led->pcdev_group[i].r.cdev;
			mutex_lock(&led_cdev->led_access);
			sunxi_set_led_brightness(led_cdev, value);
			mutex_unlock(&led_cdev->led_access);
		}
	} else if (channel%3 == 1) {
		for (i = 0; i < led->led_count; i++) {
			led_cdev = &led->pcdev_group[i].g.cdev;
			mutex_lock(&led_cdev->led_access);
			sunxi_set_led_brightness(led_cdev, value);
			mutex_unlock(&led_cdev->led_access);
		}
	} else {
		for (i = 0; i < led->led_count; i++) {
			led_cdev = &led->pcdev_group[i].b.cdev;
			mutex_lock(&led_cdev->led_access);
			sunxi_set_led_brightness(led_cdev, value);
			mutex_unlock(&led_cdev->led_access);
		}
	}
}



static int sunxi_led_probe(struct platform_device *pdev)
{
	int err;
	struct sunxi_led *led;
	struct device *dev = &pdev->dev;
	struct resource *mem_res = NULL;
	int ret;

	dprintk(DEBUG_INIT, "start\n");

	led = kzalloc(sizeof(struct sunxi_led), GFP_KERNEL);
	if (!led) {
		LED_ERR("kzalloc failed\n");
		ret = -ENOMEM;
	}

	/* global variable, edfined at the begining*/
	sunxi_led_global = led;

	platform_set_drvdata(pdev, led);
	led->dev = dev;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		LED_ERR("failed to get MEM res\n");
		ret = -ENXIO;
		goto emem;
	}

	if (!request_mem_region(mem_res->start, resource_size(mem_res),
				mem_res->name)) {
		LED_ERR("failed to request mem region\n");
		ret = -EINVAL;
		goto emem;
	}

	led->iomem_reg_base = ioremap(mem_res->start, resource_size(mem_res));
	if (!led->iomem_reg_base) {
		ret = -EIO;
		goto eiomap;
	}
	led->res = mem_res;

	led->output_mode.str = kzalloc(3, GFP_KERNEL);
	if (!led->output_mode.str) {
		LED_ERR("kzalloc failed\n");
		ret = -ENOMEM;
		goto ezalloc_str;
	}

	sunxi_get_para_of_property(led);

	err = led_regulator_request(led);
	if (err < 0) {
		LED_ERR("request regulator failed!\n");
		ret = err;
		goto eregulator;
	}

	err = sunxi_register_led_classdev(led);
	if (err) {
		LED_ERR("failed to register led classdev\n");
		ret = err;
		goto eclassdev;
	}

	sunxi_ledc_set_time(led);

	led->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(led->reset)) {
		LED_ERR("get reset clk error\n");
		return -EINVAL;
	}
	ret = reset_control_deassert(led->reset);
	if (ret) {
		LED_ERR("deassert clk error, ret:%d\n", ret);
		return ret;
	}

	sunxi_clk_init(led);

	init_waitqueue_head(&led->wait);

	err = sunxi_ledc_irq_init(led);
	if (err) {
		LED_ERR("failed to init irq\n");
		ret = err;
		goto eirq;
	}

	sunxi_ledc_pinctrl_init(led);

#ifdef CONFIG_DEBUG_FS
	sunxi_led_create_debugfs(led);
#endif /* CONFIG_DEBUG_FS */

	dprintk(DEBUG_INIT, "finish\n");
	return 0;

eirq:
	sunxi_unregister_led_classdev(led);
	sunxi_clk_deinit(led);

eclassdev:
	led_regulator_release(led);

eregulator:
	kfree(led->output_mode.str);

ezalloc_str:
	iounmap(led->iomem_reg_base);
	led->iomem_reg_base = NULL;

eiomap:
	release_mem_region(mem_res->start, resource_size(mem_res));

emem:
	kfree(led);
	return ret;
}

static int sunxi_led_remove(struct platform_device *pdev)
{
	struct sunxi_led *led = platform_get_drvdata(pdev);

#ifdef CONFIG_DEBUG_FS
	sunxi_led_remove_debugfs(led);
#endif /* CONFIG_DEBUG_FS */

	sunxi_ledc_irq_deinit(led);

	sunxi_unregister_led_classdev(led);
	sunxi_clk_deinit(led);

	led_regulator_release(led);

	kfree(led->output_mode.str);
	led->output_mode.str = NULL;

	iounmap(led->iomem_reg_base);
	led->iomem_reg_base = NULL;

	release_mem_region(led->res->start, resource_size(led->res));

	kfree(led);
	led = NULL;

	dprintk(DEBUG_INIT, "finish\n");
	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static inline void sunxi_led_save_regs(struct sunxi_led *led)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_led_regs_offset); i++)
		led->regs_backup[i] = readl(led->iomem_reg_base + sunxi_led_regs_offset[i]);
}

static inline void sunxi_led_restore_regs(struct sunxi_led *led)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_led_regs_offset); i++)
		writel(led->regs_backup[i], led->iomem_reg_base + sunxi_led_regs_offset[i]);
}

static void sunxi_led_enable_irq(struct sunxi_led *led)
{
	enable_irq(led->irqnum);
}

static void sunxi_led_disable_irq(struct sunxi_led *led)
{
	disable_irq_nosync(led->irqnum);
}

static int sunxi_led_gpio_state_select(struct sunxi_led *led, char *name)
{
	int err;
	struct pinctrl_state *pctrl_state;

	pctrl_state = pinctrl_lookup_state(led->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		dev_err(led->dev, "pinctrl_lookup_state(%s) failed! return %p\n",
				name, pctrl_state);
		return PTR_ERR(pctrl_state);
	}

	err = pinctrl_select_state(led->pctrl, pctrl_state);
	if (err < 0) {
		dev_err(led->dev, "pinctrl_select_state(%s) failed! return %d\n",
				name, err);
		return err;
	}

	return 0;
}

static void sunxi_led_enable_clk(struct sunxi_led *led)
{
	clk_prepare_enable(led->clk_ledc);
	clk_prepare_enable(led->clk_cpuapb);
}

static void sunxi_led_disable_clk(struct sunxi_led *led)
{
	clk_disable_unprepare(led->clk_cpuapb);
	clk_disable_unprepare(led->clk_ledc);
}

static int sunxi_led_power_on(struct sunxi_led *led)
{
	int err;

	if (led->regulator == NULL)
		return 0;

	err = regulator_enable(led->regulator);
	if (err) {
		dev_err(led->dev, "enable regulator %s failed!\n", led->regulator_id);
		return err;
	}
	return 0;
}

static int sunxi_led_power_off(struct sunxi_led *led)
{
	int err;

	if (led->regulator == NULL)
		return 0;

	err = regulator_disable(led->regulator);
	if (err) {
		dev_err(led->dev, "disable regulator %s failed!\n", led->regulator_id);
		return err;
	}
	return 0;
}

static int sunxi_led_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_led *led = platform_get_drvdata(pdev);

	dev_dbg(led->dev, "[%s] enter standby\n", __func__);

	sunxi_led_disable_irq(led);

	sunxi_led_save_regs(led);

	sunxi_led_gpio_state_select(led, PINCTRL_STATE_SLEEP);

	sunxi_led_disable_clk(led);

	reset_control_assert(led->reset);

	sunxi_led_power_off(led);

	return 0;
}

static int sunxi_led_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_led *led = platform_get_drvdata(pdev);

	dev_dbg(led->dev, "[%s] return from standby\n", __func__);

	sunxi_led_power_on(led);

	reset_control_deassert(led->reset);

	sunxi_led_enable_clk(led);

	sunxi_led_gpio_state_select(led, PINCTRL_STATE_DEFAULT);

	sunxi_led_restore_regs(led);

	sunxi_led_enable_irq(led);

	return 0;
}

static const struct dev_pm_ops sunxi_led_pm_ops = {
	.suspend = sunxi_led_suspend,
	.resume = sunxi_led_resume,
};

#define SUNXI_LED_PM_OPS (&sunxi_led_pm_ops)
#endif

static const struct of_device_id sunxi_led_dt_ids[] = {
	{.compatible = "allwinner,sunxi-leds"},
	{},
};

static struct platform_driver sunxi_led_driver = {
	.probe		= sunxi_led_probe,
	.remove		= sunxi_led_remove,
	.driver		= {
		.name	= "sunxi-leds",
		.owner	= THIS_MODULE,
#if IS_ENABLED(CONFIG_PM)
		.pm	= SUNXI_LED_PM_OPS,
#endif
		.of_match_table = sunxi_led_dt_ids,
	},
};

module_platform_driver(sunxi_led_driver);
module_param_named(debug, debug_mask, int, 0664);

MODULE_ALIAS("sunxi leds dirver");
MODULE_ALIAS("platform : leds dirver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.1");
MODULE_AUTHOR("Albert Yu <yuxyun@allwinnertech.com>");
MODULE_AUTHOR("liuyu <SWCliuyus@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner ledc-controller driver");
