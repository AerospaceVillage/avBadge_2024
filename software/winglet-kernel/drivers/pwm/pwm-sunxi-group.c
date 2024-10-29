/*
 * drivers/pwm/pwm-sunxi.c
 *
 * Allwinnertech pulse-width-modulation controller driver
 *
 * Copyright (C) 2015 AllWinner
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
//#define DEBUG
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include "pwm-sunxi-group.h"

#define PWM_DEBUG 0
#define PWM_NUM_MAX 4
#define PWM_BIND_NUM 2
#define PWM_PIN_STATE_ACTIVE "active"
#define PWM_PIN_STATE_SLEEP "sleep"
#define SUNXI_PWM_BIND_DEFAULT	255

#define CLEAR_LOW_BITS

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	    (((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	    (((reg) & CLRMASK(width, shift)) | (val << (shift)))

#if PWM_DEBUG
#define pwm_debug(fmt, arg...)	\
	pr_info("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define pwm_debug(msg...)
#endif

struct sunxi_pwm_config {
	unsigned int dead_time;
	unsigned int bind_pwm;
};

struct group_pwm_config {
	unsigned int group_channel;
	unsigned int group_run_count;
	unsigned int pwm_polarity;
	int pwm_period;
};

struct sunxi_pwm_hw_data {
	u32 pdzcr01_offset;	/* PWM dead zone control register 01*/
	u32 pdzcr23_offset;	/* PWM dead zone control register 23*/
	u32 pdzcr45_offset;	/* PWM dead zone control register 45*/
	u32 per_offset;		/* PWM enable register */
	u32 cer_offset;		/* PWM capture enable register */
	u32 ver_reg_offset;	/* PWM version register */
	u32 pcr_base_offset;	/* PWM control register */
	u32 ppr_base_offset;	/* PWM period register */
	u32 pcntr_base_offset;	/* PWM counter register */
	u32 ccr_base_offset;	/* PWM capture control register */
	u32 crlr_base_offset;	/* PWM capture rise lock register */
	u32 cflr_base_offset;	/* PWM capture fall lock register */
	int pm_regs_num;	/* PWM pm related register length */
	bool clk_gating_separate;	/* PWM clk gating register whether to separate */
};

struct sunxi_pwm_chip {
	struct sunxi_pwm_hw_data *data;
	u32 *regs_backup;
	u32 *pm_regs_offset;
	struct pwm_chip pwm_chip;
	void __iomem *base;
	struct sunxi_pwm_config *config;
	struct clk	*clk;
	struct reset_control	*reset;
	unsigned int group_ch;
	unsigned int group_polarity;
	unsigned int group_period;
	struct pinctrl *pctl;
	unsigned int cells_num;
};

static struct sunxi_pwm_hw_data sunxi_pwm_v100_data = {
	.pdzcr01_offset = 0x0030,
	.pdzcr23_offset = 0x0034,
	.pdzcr45_offset = 0x0038,
	.per_offset = 0x0040,
	.cer_offset = 0x0044,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0060,
	.ppr_base_offset = 0x0060 + 0x0004,
	.pcntr_base_offset = 0x0060 + 0x0008,
	.ccr_base_offset = 0x0060 + 0x000c,
	.crlr_base_offset = 0x0060 + 0x0010,
	.cflr_base_offset = 0x0060 + 0x0014,
	.clk_gating_separate = 0,
	.pm_regs_num = 4,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v200_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,

};

static inline void sunxi_pwm_save_regs(struct sunxi_pwm_chip *pc)
{
	int i;

	for (i = 0; i < pc->data->pm_regs_num; i++)
		pc->regs_backup[i] = readl(pc->base + pc->pm_regs_offset[i]);
}

static inline void sunxi_pwm_restore_regs(struct sunxi_pwm_chip *pc)
{
	int i;

	for (i = 0; i < pc->data->pm_regs_num; i++)
		writel(pc->regs_backup[i], pc->base + pc->pm_regs_offset[i]);
}

static inline struct sunxi_pwm_chip *to_sunxi_pwm_chip(struct pwm_chip *pwm_chip)
{
	return container_of(pwm_chip, struct sunxi_pwm_chip, pwm_chip);
}

static inline u32 sunxi_pwm_readl(struct pwm_chip *pwm_chip, u32 offset)
{
	u32 value;
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	value = readl(pc->base + offset);
	dev_dbg(pc->pwm_chip.dev, "%3u bytes fifo\n", value);

	return value;
}

static inline u32 sunxi_pwm_writel(struct pwm_chip *pwm_chip, u32 offset, u32 value)
{
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	writel(value, pc->base + offset);

	return 0;
}

static int sunxi_pwm_pin_set_state(struct device *dev, char *name)
{
	struct pinctrl *pctl;
	struct pinctrl_state *state = NULL;
	int err;

	pctl = devm_pinctrl_get(dev);
	if (IS_ERR(pctl)) {
		dev_err(dev, "pinctrl_get failed\n");
		err = PTR_ERR(pctl);
		return err;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		dev_err(dev, "pinctrl_lookup_state(%s) failed\n", name);
		err = PTR_ERR(state);
		goto exit;
	}

	err = pinctrl_select_state(pctl, state);
	if (err) {
		dev_err(dev, "pinctrl_select_state(%s) failed\n", name);
		goto exit;
	}

exit:
	devm_pinctrl_put(pctl);
	return err;

}

static int sunxi_pwm_get_config(struct platform_device *pdev,
				struct sunxi_pwm_config *config)
{
	int err;
	struct device_node *np;

	np = pdev->dev.of_node;

	err = of_property_read_u32(np, "bind_pwm", &config->bind_pwm);
	if (err < 0) {
		/*if there is no bind pwm,set 255, dual pwm invalid!*/
		config->bind_pwm = SUNXI_PWM_BIND_DEFAULT;
		err = 0;
	}

	err = of_property_read_u32(np, "dead_time", &config->dead_time);
	if (err < 0) {
		/*if there is  bind pwm, but not set dead time,set bind pwm 255,dual pwm invalid!*/
		config->bind_pwm = SUNXI_PWM_BIND_DEFAULT;
		err = 0;
	}

	of_node_put(np);

	return err;
}

static int sunxi_pwm_set_polarity_single(struct pwm_chip *pwm_chip,
					struct pwm_device *pwm,
					enum pwm_polarity polarity)
{
	u32 temp;
	unsigned int reg_offset, reg_shift, reg_width;
	u32 sel = 0;
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	sel = pwm->pwm - pwm_chip->base;
	reg_offset = pc->data->pcr_base_offset + sel * PWM_REG_UNIFORM_OFFSET;
	reg_shift = PWM_ACT_STA_SHIFT;
	reg_width = PWM_ACT_STA_WIDTH;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (polarity == PWM_POLARITY_NORMAL) /* set single polarity*/
		temp = SET_BITS(reg_shift, reg_width, temp, 1);
	else
		temp = SET_BITS(reg_shift, reg_width, temp, 0);

	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

	return 0;
}

static int sunxi_pwm_set_polarity_dual(struct pwm_chip *pwm_chip,
					struct pwm_device *pwm,
					enum pwm_polarity polarity,
					int bind_num)
{
	u32 temp[2];
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	u32 sel[2] = {0};
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	sel[0] = pwm->pwm - pwm_chip->base;
	sel[1] = bind_num - pwm_chip->base;
	/* config current pwm*/
	reg_offset[0] = pc->data->pcr_base_offset + sel[0] * PWM_REG_UNIFORM_OFFSET;
	reg_shift[0] = PWM_ACT_STA_SHIFT;
	reg_width[0] = PWM_ACT_STA_WIDTH;
	temp[0] = sunxi_pwm_readl(pwm_chip, reg_offset[0]);
	if (polarity == PWM_POLARITY_NORMAL)
		temp[0] = SET_BITS(reg_shift[0], 1, temp[0], 1);
	else
		temp[0] = SET_BITS(reg_shift[0], 1, temp[0], 0);
	/* config bind pwm*/
	reg_offset[1] = pc->data->pcr_base_offset + sel[1] * PWM_REG_UNIFORM_OFFSET;
	reg_shift[1] = PWM_ACT_STA_SHIFT;
	reg_width[1] = PWM_ACT_STA_WIDTH;
	temp[1] = sunxi_pwm_readl(pwm_chip, reg_offset[1]);

	/*bind pwm's polarity is reverse compare with the  current pwm*/
	if (polarity == PWM_POLARITY_NORMAL)
		temp[1] = SET_BITS(reg_shift[0], 1, temp[1], 1);
	else
		temp[1] = SET_BITS(reg_shift[0], 1, temp[1], 0);
	/*config register at the same time*/
	sunxi_pwm_writel(pwm_chip, reg_offset[0], temp[0]);
	sunxi_pwm_writel(pwm_chip, reg_offset[1], temp[1]);

	return 0;
}

static int sunxi_pwm_set_polarity(struct pwm_chip *pwm_chip, struct pwm_device *pwm,
				enum pwm_polarity polarity)
{
	int bind_num;
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	bind_num = pc->config[pwm->pwm - pwm_chip->base].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		sunxi_pwm_set_polarity_single(pwm_chip, pwm, polarity);
	else
		sunxi_pwm_set_polarity_dual(pwm_chip, pwm, polarity, bind_num);

	return 0;
}


static int get_pccr_reg_offset(u32 sel, u32 *reg_offset)
{
	switch (sel) {
	case 0:
	case 1:
		*reg_offset = PWM_PCCR01;
		break;
	case 2:
	case 3:
		*reg_offset = PWM_PCCR23;
		break;
	case 4:
	case 5:
		*reg_offset = PWM_PCCR45;
		break;
	case 6:
	case 7:
		*reg_offset = PWM_PCCR67;
		break;
	case 8:
	case 9:
		*reg_offset = PWM_PCCR89;
		break;
	case 10:
	case 11:
		*reg_offset = PWM_PCCRAB;
		break;
	case 12:
	case 13:
		*reg_offset = PWM_PCCRCD;
		break;
	case 14:
	case 15:
		*reg_offset = PWM_PCCREF;
		break;
	default:
		pr_err("%s:Not supported!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static int get_pdzcr_reg_offset(struct sunxi_pwm_chip *pc, u32 sel, u32 *reg_offset)
{
	switch (sel) {
	case 0:
	case 1:
		*reg_offset = pc->data->pdzcr01_offset;
		break;
	case 2:
	case 3:
		*reg_offset = pc->data->pdzcr23_offset;
		break;
	case 4:
	case 5:
		*reg_offset = pc->data->pdzcr45_offset;
		break;
	case 6:
	case 7:
		*reg_offset = PWM_PDZCR67;
		break;
	case 8:
	case 9:
		*reg_offset = PWM_PDZCR89;
		break;
	case 10:
	case 11:
		*reg_offset = PWM_PDZCRAB;
		break;
	case 12:
	case 13:
		*reg_offset = PWM_PDZCRCD;
		break;
	case 14:
	case 15:
		*reg_offset = PWM_PDZCREF;
		break;
	default:
		pr_err("%s:Not supported!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

#define PRESCALE_MAX 256

static int sunxi_pwm_config_single(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		int duty_ns, int period_ns)
{
	unsigned int temp;
	unsigned long long c = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned int reg_bypass_shift;
	unsigned int reg_clk_src_shift, reg_clk_src_width;
	unsigned int reg_div_m_shift, reg_div_m_width, value;
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	unsigned int reg_clk_gating_shift, reg_clk_gating_width;
	unsigned int pwm_run_count = 0;
	int err;
	struct sunxi_pwm_chip *pc;
	struct group_pwm_config *pdevice;
	u32 sel = 0;
	u32 pre_scal[][2] = {

		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	pc = to_sunxi_pwm_chip(pwm_chip);
	pdevice = pwm_device->chip_data;

	if (pwm_device->chip_data) {
		pwm_run_count = pdevice->group_run_count;
		pc->group_ch = pdevice->group_channel;
		pc->group_polarity = pdevice->pwm_polarity;
		pc->group_period = pdevice->pwm_period;
	}

	if (pc->group_ch) {
		reg_offset = pc->data->per_offset;
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value &= ~((0xf) << 4*(pc->group_ch - 1));
		sunxi_pwm_writel(pwm_chip, reg_offset, value);
	}

	sel = pwm_device->pwm - pwm_chip->base;
	/* sel / 2 * 0x04 + 0x20  */
	err = get_pccr_reg_offset(sel, &reg_offset);
	if (err) {
		dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
		return -EINVAL;
	}

	/* src clk reg */
	reg_clk_src_shift = PWM_CLK_SRC_SHIFT;
	reg_clk_src_width = PWM_CLK_SRC_WIDTH;

	if (pc->group_ch) {
		/* group_mode used the apb1 clk */
		temp = sunxi_pwm_readl(pwm_chip, reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
		sunxi_pwm_writel(pwm_chip, reg_offset, temp);
	} else {
		if (period_ns > 0 && period_ns <= 10) {
			/* if freq lt 100M, then direct output 100M clock,set by pass. */
			c = 100000000;
			/* config  bypass */
			if (!pc->data->clk_gating_separate) {
				if ((sel % 2) == 0)
					reg_bypass_shift = PWM_BYPASS_SHIFT;
				else
					reg_bypass_shift = PWM_BYPASS_SHIFT + 1;
				temp = sunxi_pwm_readl(pwm_chip, reg_offset);
				temp = SET_BITS(reg_bypass_shift, PWM_BYPASS_WIDTH, temp, 1); /* sun50iw9 bypass set */
				sunxi_pwm_writel(pwm_chip, reg_offset, temp);
			} else {
				reg_bypass_shift = sel;
				temp = sunxi_pwm_readl(pwm_chip, PWM_PCGR);
				temp = SET_BITS(reg_bypass_shift, PWM_BYPASS_WIDTH, temp, 1); /* bypass set */
				sunxi_pwm_writel(pwm_chip, PWM_PCGR, temp);
			}

			/* clk_src_reg */
			temp = sunxi_pwm_readl(pwm_chip, reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);/*clock source*/
			sunxi_pwm_writel(pwm_chip, reg_offset, temp);

			return 0;
		} else if (period_ns > 10 && period_ns <= 334) {
			/* if freq between 3M~100M, then select 100M as clock */
			c = 100000000;
			/* clk_src_reg */
			temp = sunxi_pwm_readl(pwm_chip, reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);
			sunxi_pwm_writel(pwm_chip, reg_offset, temp);

		} else if (period_ns > 334) {
			/* if freq < 3M, then select 24M clock */
			c = 24000000;
			/* clk_src_reg */
			temp = sunxi_pwm_readl(pwm_chip, reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
			sunxi_pwm_writel(pwm_chip, reg_offset, temp);
		}
		pwm_debug("duty_ns=%d period_ns=%d c =%llu.\n", duty_ns, period_ns, c);

		c = c * period_ns;
		do_div(c, 1000000000);
		entire_cycles = (unsigned long)c; /* How many clksrc beats in a PWM period */

		/* get entire cycle length */
		for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
			if (entire_cycles <= 65536)
				break;
			for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
				entire_cycles = ((unsigned long)c/pre_scal[pre_scal_id][1])/(prescale + 1);
				if (entire_cycles <= 65536) {
					div_m = pre_scal[pre_scal_id][0];
					break;
				}
			}
		}
		/* get active cycles/high level time */
		/* active_cycles = entire_cycles * duty_ns / period_ns  */
		c = (unsigned long long)entire_cycles * duty_ns;
		do_div(c, period_ns);
		active_cycles = c;
		if (entire_cycles == 0)
			entire_cycles++;
	}

	/* config  clk div_m */
	reg_div_m_shift = PWM_DIV_M_SHIFT;
	reg_div_m_width = PWM_DIV_M_WIDTH;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (pc->group_ch)
		temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, 0);
	else
		temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, div_m);
	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

	/* config clk gating */
	if (!pc->data->clk_gating_separate) {
		err = get_pccr_reg_offset(sel, &reg_offset);
		if (err) {
			dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
			return -EINVAL;
		}

		reg_clk_gating_shift = PWM_CLK_GATING_SHIFT;
		reg_clk_gating_width = PWM_CLK_GATING_WIDTH;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset);
		temp = SET_BITS(reg_clk_gating_shift, reg_clk_gating_width, temp, 1);
		sunxi_pwm_writel(pwm_chip, reg_offset, temp);
	} else {
		reg_shift = sel;
		value = sunxi_pwm_readl(pwm_chip, PWM_PCGR);
		value = SET_BITS(reg_shift, 1, value, 1);/* set gating */
		sunxi_pwm_writel(pwm_chip, PWM_PCGR, value);
	}

#if defined(CONFIG_ARCH_SUN50IW9) && defined(CONFIG_MFD_ACX00)
	/* use pwm5 as phy(ac300,gmac) clk */
	if (sel == 5) {
		reg_shift = sel + 1;
		value = sunxi_pwm_readl(pwm_chip, PWM_PCCR45);
		value = SET_BITS(reg_shift, 1, value, 1);
		sunxi_pwm_writel(pwm_chip, PWM_PCCR45, value);

		reg_shift = sel - 1;
		value = sunxi_pwm_readl(pwm_chip, PWM_PCCR45);
		value = SET_BITS(reg_shift, 1, value, 1);
		sunxi_pwm_writel(pwm_chip, PWM_PCCR45, value);
	}
#endif

	/* config prescal_k */
	reg_offset = pc->data->pcr_base_offset + PWM_REG_UNIFORM_OFFSET * sel;
	reg_shift = PWM_PRESCAL_SHIFT;
	reg_width = PWM_PRESCAL_WIDTH;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (pc->group_ch)
		temp = SET_BITS(reg_shift, reg_width, temp, 0xef);
	else
		temp = SET_BITS(reg_shift, reg_width, temp, prescale);
	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

	if (pc->group_ch) {
		/* group set enable */
		reg_offset = PWM_PGR0 + 0x04 * (pc->group_ch - 1);
		reg_shift = sel;
		reg_width = 1;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, 1); /* set  group0_cs */
		sunxi_pwm_writel(pwm_chip, reg_offset, temp);

		/* pwm pulse mode set */
		reg_offset = pc->data->pcr_base_offset + sel * PWM_REG_UNIFORM_OFFSET;
		reg_shift = PWM_MODE_ACTS_SHIFT;
		reg_width = PWM_MODE_ACTS_WIDTH;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, 0x3); /* pwm pulse mode and active */
		/* pwm output pulse num */
		reg_shift = PWM_PUL_NUM_SHIFT;
		reg_width = PWM_PUL_NUM_WIDTH;
		temp = SET_BITS(reg_shift, reg_width, temp, pwm_run_count);   /* pwm output pulse num */
		sunxi_pwm_writel(pwm_chip, reg_offset, temp);
	}

	/* config active cycles num */
	reg_offset = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * sel;
	reg_shift = PWM_ACT_CYCLES_SHIFT;
	reg_width = PWM_ACT_CYCLES_WIDTH;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (pc->group_ch)
		temp = SET_BITS(reg_shift, reg_width, temp,
				(unsigned int)((pc->group_period * 3) >> 3));
	else
		temp = SET_BITS(reg_shift, reg_width, temp, active_cycles);
	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

	/* config period cycles num */
	reg_offset = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * sel;
	reg_shift = PWM_PERIOD_CYCLES_SHIFT;
	reg_width = PWM_PERIOD_CYCLES_WIDTH;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (pc->group_ch) {
		temp = SET_BITS(reg_shift, reg_width, temp, pc->group_period);
		pc->group_ch = 0;
	} else
		temp = SET_BITS(reg_shift, reg_width, temp, (entire_cycles - 1));
	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

	pwm_debug("active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);
	return 0;
}

static int sunxi_pwm_config_dual(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		int duty_ns, int period_ns, int bind_num)
{
	u32 value[2] = {0};
	unsigned int temp;
	unsigned long long c = 0, clk = 0, clk_temp = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	unsigned int reg_bypass_shift;
	unsigned int reg_dz_en_offset[2], reg_dz_en_shift[2], reg_dz_en_width[2];
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	unsigned int pwm_index[2] = {0};
	int src_clk_sel = 0;
	int i = 0;
	int err;
	unsigned int dead_time = 0, duty = 0;
	struct sunxi_pwm_chip *pc;
	u32 pre_scal[][2] = {

		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	pc = to_sunxi_pwm_chip(pwm_chip);

	pwm_index[0] = pwm_device->pwm - pwm_chip->base;
	pwm_index[1] = bind_num - pwm_chip->base;

	/* if duty time < dead time,it is wrong. */
	dead_time = pc->config[pwm_index[0]].dead_time;
	duty = (unsigned int)duty_ns;
	/* judge if the pwm eanble dead zone */
	err = get_pdzcr_reg_offset(pc, pwm_index[0], &reg_dz_en_offset[0]);
	if (err) {
		dev_err(pc->pwm_chip.dev, "get pwm dead zone failed\n");
		return -EINVAL;
	}

	reg_dz_en_shift[0] = PWM_DZ_EN_SHIFT;
	reg_dz_en_width[0] = PWM_DZ_EN_WIDTH;

	value[0] = sunxi_pwm_readl(pwm_chip, reg_dz_en_offset[0]);
	value[0] = SET_BITS(reg_dz_en_shift[0], reg_dz_en_width[0], value[0], 1);
	sunxi_pwm_writel(pwm_chip, reg_dz_en_offset[0], value[0]);
	temp = sunxi_pwm_readl(pwm_chip, reg_dz_en_offset[0]);
	temp &=  (1u << reg_dz_en_shift[0]);
	if (duty < dead_time || temp == 0) {
		pr_err("[PWM]duty time or dead zone error.\n");
		return -EINVAL;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		if ((i % 2) == 0)
			reg_bypass_shift = 0x5;
		else
			reg_bypass_shift = 0x6;

		err = get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		if (err) {
			dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
			return -EINVAL;
		}

		reg_shift[i] = reg_bypass_shift;
		reg_width[i] = PWM_BYPASS_WIDTH;
	}

	if (period_ns > 0 && period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass */
		clk = 100000000;
		src_clk_sel = 1;

		/* config the two pwm bypass */
		for (i = 0; i < PWM_BIND_NUM; i++) {
			temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
			temp = SET_BITS(reg_shift[i], reg_width[i], temp, 1);
			sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);

			reg_shift[i] = PWM_CLK_SRC_SHIFT;
			reg_width[i] = PWM_CLK_SRC_WIDTH;
			temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
			temp = SET_BITS(reg_shift[i], reg_width[i], temp, 1);
			sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);
		}

		return 0;
	} else if (period_ns > 10 && period_ns <= 334) {
		clk = 100000000;
		src_clk_sel = 1;
	} else if (period_ns > 334) {
		/* if freq < 3M, then select 24M clock */
		clk = 24000000;
		src_clk_sel = 0;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift[i] = PWM_CLK_SRC_SHIFT;
		reg_width[i] = PWM_CLK_SRC_WIDTH;

		temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, src_clk_sel);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);
	}

	c = clk;
	c *= period_ns;
	do_div(c, 1000000000);
	entire_cycles = (unsigned long)c;

	/* get div_m and prescale,which satisfy: deat_val <= 256, entire <= 65536 */
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
			entire_cycles = ((unsigned long)c/pre_scal[pre_scal_id][1])/(prescale + 1);
			clk_temp = clk;
			do_div(clk_temp, pre_scal[pre_scal_id][1] * (prescale + 1));
			clk_temp *= dead_time;
			do_div(clk_temp, 1000000000);
			if (entire_cycles <= 65536 && clk_temp <= 256) {
				div_m = pre_scal[pre_scal_id][0];
				break;
			}
		}
		if (entire_cycles <= 65536 && clk_temp <= 256)
				break;
		else {
			pr_err("%s:config dual err.entire_cycles=%lu, dead_zone_val=%llu",
					__func__, entire_cycles, clk_temp);
			return -EINVAL;
		}
	}

	c = (unsigned long long)entire_cycles * duty_ns;
	do_div(c,  period_ns);
	active_cycles = c;
	if (entire_cycles == 0)
		entire_cycles++;

	/* config  clk div_m */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift[i] = PWM_DIV_M_SHIFT;
		reg_width[i] = PWM_DIV_M_SHIFT;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, div_m);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);
	}

	/* config prescal */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->pcr_base_offset + PWM_REG_UNIFORM_OFFSET * pwm_index[i];
		reg_shift[i] = PWM_PRESCAL_SHIFT;
		reg_width[i] = PWM_PRESCAL_WIDTH;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, prescale);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);
	}

	/* config active cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * pwm_index[i];
		reg_shift[i] = PWM_ACT_CYCLES_SHIFT;
		reg_width[i] = PWM_ACT_CYCLES_WIDTH;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, active_cycles);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);
	}

	/* config period cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * pwm_index[i];
		reg_shift[i] = PWM_PERIOD_CYCLES_SHIFT;
		reg_width[i] = PWM_PERIOD_CYCLES_WIDTH;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, (entire_cycles - 1));
		sunxi_pwm_writel(pwm_chip, reg_offset[i], temp);
	}

	pwm_debug("active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);

	/* config dead zone, one config for two pwm */
	reg_offset[0] = reg_dz_en_offset[0];
	reg_shift[0] = PWM_PDZINTV_SHIFT;
	reg_width[0] = PWM_PDZINTV_WIDTH;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset[0]);
	temp = SET_BITS(reg_shift[0], reg_width[0], temp, (unsigned int)clk_temp);
	sunxi_pwm_writel(pwm_chip, reg_offset[0], temp);

	return 0;
}

static int sunxi_pwm_config_channel(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		int duty_ns, int period_ns)
{
	int ret;
	int bind_num;

	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	bind_num = pc->config[pwm_device->pwm - pwm_chip->base].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		ret = sunxi_pwm_config_single(pwm_chip, pwm_device, duty_ns, period_ns);
	else
		ret = sunxi_pwm_config_dual(pwm_chip, pwm_device, duty_ns, period_ns, bind_num);

	return ret;
}

static int sunxi_pwm_enable_single(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	unsigned int value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width, group_reg_offset;
	unsigned int temp;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;
	static unsigned int enable_num;
	unsigned int pwm_start_count, i;
	int pwm_period = 0;
	int err;
	struct sunxi_pwm_chip *pc;
	struct group_pwm_config *pdevice;

	pc = to_sunxi_pwm_chip(pwm_chip);
	pdevice = pwm_device->chip_data;

	index = pwm_device->pwm - pwm_chip->base;
	sub_np = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", index);
	if (!sub_np) {
		pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (!pwm_pdevice) {
		pr_err("%s: can't parse pwm device\n", __func__);
		return -ENODEV;
	}
	err = sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);
	if (err != 0)
		return err;

	if (pwm_device->chip_data) {
		pc->group_ch = pdevice->group_channel;
		pwm_period = pdevice->pwm_period;
	}

	if (pc->group_ch)
		enable_num++;

	/* enable pwm controller  pwm can be used */
	if (!pc->group_ch) {
		/* pwm channel enable */
		reg_offset = pc->data->per_offset;
		reg_shift = index;
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value = SET_BITS(reg_shift, 1, value, 1);
		sunxi_pwm_writel(pwm_chip, reg_offset, value);

		/* config clk gating */
		if (!pc->data->clk_gating_separate) {
			err = get_pccr_reg_offset(index, &reg_offset);
			if (err) {
				dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
				return -EINVAL;
			}

			reg_shift = PWM_CLK_GATING_SHIFT;
			reg_width = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset = PWM_PCGR;
			reg_shift = index;
			reg_width = 0x1;
		}
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value = SET_BITS(reg_shift, reg_width, value, 1);
		sunxi_pwm_writel(pwm_chip, reg_offset, value);
	}

	if (pc->group_ch && enable_num == 4) {
		if (pc->group_polarity)
			pwm_start_count = (unsigned int)pwm_period*6/8;
		else
			pwm_start_count = 0;

		for (i = 4 * (pc->group_ch - 1); i < 4 * pc->group_ch; i++) {
			/* start count set */
			reg_offset = pc->data->pcntr_base_offset + PWM_REG_UNIFORM_OFFSET * i;
			reg_shift = PWM_COUNTER_START_SHIFT;
			reg_width = PWM_COUNTER_START_WIDTH;

			temp = pwm_start_count << reg_shift;
			sunxi_pwm_writel(pwm_chip, reg_offset, temp);
			if (pc->group_polarity)
				pwm_start_count = pwm_start_count -
					((unsigned int)pwm_period*2/8);
			else
				pwm_start_count = pwm_start_count +
					((unsigned int)pwm_period*2/8);
		}

		/* pwm channel enable */
		reg_offset = pc->data->per_offset;
		reg_shift = index;
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value |= ((0xf) << 4*(pc->group_ch - 1));
		sunxi_pwm_writel(pwm_chip, reg_offset, value);

		/* pwm group control */
		group_reg_offset = PWM_PGR0 + 0x04 * (pc->group_ch - 1);

		enable_num = 0;
		pwm_start_count = 0;
		/* group en and start */
		reg_shift = PWMG_EN_SHIFT;
		value = sunxi_pwm_readl(pwm_chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 1);/* enable group0 enable */
		sunxi_pwm_writel(pwm_chip, group_reg_offset, value);

		reg_shift = PWMG_START_SHIFT;
		value = sunxi_pwm_readl(pwm_chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 1);/* group0 start */
		sunxi_pwm_writel(pwm_chip, group_reg_offset, value);

		pc->group_ch = 0;
	}

	return 0;
}

static int sunxi_pwm_enable_dual(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device, int bind_num)
{
	u32 value[2] = {0};
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i, err;
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	pwm_index[0] = pwm_device->pwm - pwm_chip->base;
	pwm_index[1] = bind_num - pwm_chip->base;

	/* set current pwm pin state */
	sub_np[0] = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", pwm_index[0]);
	if (IS_ERR_OR_NULL(sub_np[0])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[0] = of_find_device_by_node(sub_np[0]);
	if (IS_ERR_OR_NULL(pwm_pdevice[0])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	/* set bind pwm pin state */
	sub_np[1] = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", pwm_index[1]);
	if (IS_ERR_OR_NULL(sub_np[1])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[1] = of_find_device_by_node(sub_np[1]);
	if (IS_ERR_OR_NULL(pwm_pdevice[1])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	err = sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_ACTIVE);
	if (err)
		return err;
	err = sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_ACTIVE);
	if (err)
		return err;

	/* enable clk for pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		err = get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		if (err) {
			dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
			return -EINVAL;
		}

		reg_shift[i] = PWM_CLK_GATING_SHIFT;
		reg_width[i] = PWM_CLK_GATING_WIDTH;
		value[i] = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 1);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], value[i]);
	}

	/* enable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->per_offset;
		reg_shift[i] = pwm_index[i];
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 1);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], value[i]);
	}

	return 0;
}

static int sunxi_pwm_enable(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	int bind_num;
	int ret;
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	bind_num = pc->config[pwm_device->pwm - pwm_chip->base].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		ret = sunxi_pwm_enable_single(pwm_chip, pwm_device);
	else
		ret = sunxi_pwm_enable_dual(pwm_chip, pwm_device, bind_num);

	return ret;
}

static int sunxi_pwm_disable_single(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	u32 value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width, group_reg_offset;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;
	int err;

	static int disable_num;
	struct sunxi_pwm_chip *pc;
	struct group_pwm_config *pdevice;

	pc = to_sunxi_pwm_chip(pwm_chip);
	pdevice = pwm_device->chip_data;

	index = pwm_device->pwm - pwm_chip->base;

	if (pwm_device->chip_data)
		pc->group_ch = pdevice->group_channel;

	/* disable pwm controller */
	if (pc->group_ch) {
		if (disable_num == 0) {
			reg_offset = pc->data->per_offset;
			reg_width = 0x4;
			value = sunxi_pwm_readl(pwm_chip, reg_offset);
			value &= ~((0xf) << 4*(pc->group_ch - 1));
			sunxi_pwm_writel(pwm_chip, reg_offset, value);
			/* config clk gating */
			if (!pc->data->clk_gating_separate) {
				err = get_pccr_reg_offset(index, &reg_offset);
				if (err) {
					dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
					return -EINVAL;
				}

				reg_shift = PWM_CLK_GATING_SHIFT;
				reg_width = PWM_CLK_GATING_WIDTH;
			} else {
				reg_offset = PWM_PCGR;
				reg_shift = index;
				reg_width = 0x1;
			}
			value = sunxi_pwm_readl(pwm_chip, reg_offset);
			value &= ~((0xf) << 4*(pc->group_ch - 1));
			//	value = SET_BITS(reg_shift, reg_width, value, 0);
			sunxi_pwm_writel(pwm_chip, reg_offset, value);
		}
	} else {
		reg_offset = pc->data->per_offset;
		reg_shift = index;
		reg_width = 0x1;
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value = SET_BITS(reg_shift, reg_width, value, 0);
		sunxi_pwm_writel(pwm_chip, reg_offset, value);

		/* config clk gating */
		if (!pc->data->clk_gating_separate) {
			err = get_pccr_reg_offset(index, &reg_offset);
			if (err) {
				dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
				return -EINVAL;
			}

			reg_shift = PWM_CLK_GATING_SHIFT;
			reg_width = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset = PWM_PCGR;
			reg_shift = index;
			reg_width = 0x1;
		}
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value = SET_BITS(reg_shift, reg_width, value, 0);
		sunxi_pwm_writel(pwm_chip, reg_offset, value);
	}

	if (pc->group_ch)
		disable_num++;

	sub_np = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		pr_err("%s: can't parse pwm device\n", __func__);
		return -ENODEV;
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);

	if (pc->group_ch) {
		group_reg_offset = PWM_PGR0 + 0x04 * (pc->group_ch - 1);
		/* group end */
		reg_shift = PWMG_START_SHIFT;
		value = sunxi_pwm_readl(pwm_chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 0);/* group end */
		sunxi_pwm_writel(pwm_chip, group_reg_offset, value);

		/* group disable */
		reg_shift = PWMG_EN_SHIFT;
		value = sunxi_pwm_readl(pwm_chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 0);/* group disable */
		sunxi_pwm_writel(pwm_chip, group_reg_offset, value);

		pc->group_ch = 0;
	}

	return 0;
}

static int sunxi_pwm_disable_dual(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device, int bind_num)
{
	u32 value[2] = {0};
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i = 0;
	int err;
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	pwm_index[0] = pwm_device->pwm - pwm_chip->base;
	pwm_index[1] = bind_num - pwm_chip->base;

	/* get current index pwm device */
	sub_np[0] = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", pwm_index[0]);
	if (IS_ERR_OR_NULL(sub_np[0])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[0] = of_find_device_by_node(sub_np[0]);
	if (IS_ERR_OR_NULL(pwm_pdevice[0])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}
	/* get bind pwm device */
	sub_np[1] = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", pwm_index[1]);
	if (IS_ERR_OR_NULL(sub_np[1])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[1] = of_find_device_by_node(sub_np[1]);
	if (IS_ERR_OR_NULL(pwm_pdevice[1])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	/* disable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->per_offset;
		reg_shift[i] = pwm_index[i];
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 0);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], value[i]);
	}

	/* disable pwm clk gating */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		err = get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		if (err) {
			dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
			return -EINVAL;
		}

		reg_shift[i] = PWM_CLK_GATING_SHIFT;
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 0);
		sunxi_pwm_writel(pwm_chip, reg_offset[i], value[i]);
	}

	/* disable pwm dead zone,one for the two pwm */
	err = get_pdzcr_reg_offset(pc, pwm_index[0], &reg_offset[0]);
	if (err) {
		dev_err(pc->pwm_chip.dev, "get pwm dead zone failed\n");
		return -EINVAL;
	}

	reg_shift[0] = PWM_DZ_EN_SHIFT;
	reg_width[0] = PWM_DZ_EN_WIDTH;
	value[0] = sunxi_pwm_readl(pwm_chip, reg_offset[0]);
	value[0] = SET_BITS(reg_shift[0], reg_width[0], value[0], 0);
	sunxi_pwm_writel(pwm_chip, reg_offset[0], value[0]);

	/* config pin sleep */
	sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_SLEEP);
	sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_SLEEP);

	return 0;
}

static int sunxi_pwm_disable(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	int bind_num;
	struct sunxi_pwm_chip *pc;
	int ret;

	pc = to_sunxi_pwm_chip(pwm_chip);

	bind_num = pc->config[pwm_device->pwm - pwm_chip->base].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		ret = sunxi_pwm_disable_single(pwm_chip, pwm_device);
	else
		ret = sunxi_pwm_disable_dual(pwm_chip, pwm_device, bind_num);

	return ret;
}

//TODO:  use pwm interrupt
/* Some soc have not interruput soure.
 * So,use CPU polling pwm capture interrupt status
 * default:24MHz
 * max input pwm period:2.7ms
 * min input pwm period:2.7us
 */
static int sunxi_pwm_capture(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		struct pwm_capture *result, unsigned long timeout)
{
	unsigned long long pwm_clk = 0, temp_clk;
	unsigned int pwm_div;
	unsigned int i = 0;
	/* spinlock_t pwm_lock; */
	/* unsigned long flags; */
	int cap_time[3];
	unsigned int value = 0, temp = 0, irq_num = 0;
	unsigned int reg_offset, reg_shift;
	int err;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;
	struct sunxi_pwm_chip *pc;
	int index = pwm_device->pwm - pwm_chip->base;
	u32 pre_scal[][2] = {
		/* reg_value  clk_pre_div */
		{0, 1},
		{1, 2},
		{2, 4},
		{3, 8},
		{4, 16},
		{5, 32},
		{6, 64},
		{7, 128},
		{8, 256},
	};

	pc = to_sunxi_pwm_chip(pwm_chip);

	sub_np = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		pr_err("%s: can't parse \"pwms\" property\n", __func__);
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		pr_err("%s: can't parse pwm device\n", __func__);
		return -ENODEV;
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);

	/* enable clk for pwm controller */
	err = get_pccr_reg_offset(index, &reg_offset);
	if (err) {
		dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
		return -EINVAL;
	}

	reg_shift = PWM_CLK_GATING_SHIFT;
	value = sunxi_pwm_readl(pwm_chip, reg_offset);
	value = SET_BITS(reg_shift, 1, value, 1);
	sunxi_pwm_writel(pwm_chip, reg_offset, value);

	err = get_pccr_reg_offset(index, &reg_offset);
	if (err) {
		dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
		return -EINVAL;
	}

	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	pwm_div = pre_scal[temp & (0x000f)][1];
	if (temp & (0x01 << PWM_CLK_SRC_SHIFT))
		pwm_clk = 100;//100M
	else
		pwm_clk = 24;//24M

	/* spin_lock_init(&pwm_lock); */
	/* spin_lock_irqsave(&pwm_lock, flags); */

	/* enable rise interrupt */
	temp = sunxi_pwm_readl(pwm_chip, PWM_CIER);
	temp = SET_BITS(index * 0x2, 0x1, temp, 0x1);
	sunxi_pwm_writel(pwm_chip, PWM_CIER, temp);
	/* Enable capture */
	temp = sunxi_pwm_readl(pwm_chip, pc->data->cer_offset);
	temp = SET_BITS(index, 0x1, temp, 0x1);
	sunxi_pwm_writel(pwm_chip, pc->data->cer_offset, temp);
	/* Clean capture rise status*/
	temp = sunxi_pwm_readl(pwm_chip, PWM_CISR);
	temp = SET_BITS(index * 0x2, 0x1, temp, 0x1);
	sunxi_pwm_writel(pwm_chip, PWM_CISR, temp);

	/* Enable rising and falling edge capture triggers */
	sunxi_pwm_writel(pwm_chip, pc->data->ccr_base_offset + index * PWM_REG_UNIFORM_OFFSET, 0x6);

	printk("time out is %ld\n", timeout);
	while (--timeout) {
		for (i = 0; i < 65535; i++) {
		/*
		 * Capture input:
		 *          _______               _______
		 *         |       |             |       |
		 * ________|       |_____________|       |________
		 *irq_num ^0      ^1                ^2
		 *
		 * Capture start by the first available rising edge.
		 */
		temp = sunxi_pwm_readl(pwm_chip, PWM_CISR);

		/* If the rising edge and the falling edge are triggered at the same time,
		 * it will be regarded as an error signal
		 */
		if ((temp & (0x1 << (index * 0x2))) &&
			(temp & (0x2 << (index * 0x2)))) {
			pr_err("input signal is constant of greater than Hz\n");
			goto err;
		}
		if (temp & (0x1 << (index * 0x2))) {
			if (irq_num == 1) {
				pr_err("pwm high time too short,can not capture,index:%d\n",
					irq_num);
				goto err;
			}
			cap_time[irq_num] = sunxi_pwm_readl(pwm_chip,
						pc->data->crlr_base_offset + index * PWM_REG_UNIFORM_OFFSET);
			irq_num++;

			/* clean irq status*/
			temp = sunxi_pwm_readl(pwm_chip, PWM_CISR);
			temp = SET_BITS(index * 0x2, 0x1, temp, 0x1);
			sunxi_pwm_writel(pwm_chip, PWM_CISR, temp);
			/* clean capture crlf */
			sunxi_pwm_writel(pwm_chip,
					pc->data->ccr_base_offset + index * PWM_REG_UNIFORM_OFFSET, 0x6);

			/* enable fail interrupt */
			temp = sunxi_pwm_readl(pwm_chip, PWM_CIER);
			temp = SET_BITS(1 + index * 0x2, 0x1, temp, 0x1);
			sunxi_pwm_writel(pwm_chip, PWM_CIER, temp);
		} else if (temp & (0x2 << (index * 0x2))) {
			if (irq_num == 0 || irq_num == 2) {
				pr_err("pwm low time too short,can not capture, index:%d\n",
					irq_num);
				goto err;
			}
			cap_time[irq_num] = sunxi_pwm_readl(pwm_chip,
					pc->data->cflr_base_offset + index * PWM_REG_UNIFORM_OFFSET);
			irq_num++;

			/* clean irq status */
			temp = sunxi_pwm_readl(pwm_chip, PWM_CISR);
			temp = SET_BITS(1 + index * 0x2, 0x1, temp, 0x1);
			sunxi_pwm_writel(pwm_chip, PWM_CISR, temp);
			/* clean capture cflf */
			sunxi_pwm_writel(pwm_chip,
					pc->data->ccr_base_offset + index * PWM_REG_UNIFORM_OFFSET, 0x2);
		}
		if (irq_num > 2) {
err:
			/* spin_unlock_irqrestore(&pwm_lock, flags); */
			/* disable fail interrupt */
			temp = sunxi_pwm_readl(pwm_chip, PWM_CIER);
			temp = SET_BITS(1 + index * 0x2, 0x1, temp, 0x0);
			sunxi_pwm_writel(pwm_chip, PWM_CIER, temp);

			/* disable capture */
			temp = sunxi_pwm_readl(pwm_chip, pc->data->cer_offset);
			temp = SET_BITS(index, 0x1, temp, 0x0);
			sunxi_pwm_writel(pwm_chip, pc->data->cer_offset, temp);
			goto end;
		}
		}
	}
end:
	/* get period and duty_cycle */
	temp_clk = (cap_time[1] + cap_time[2]) * 1000 * pwm_div;
	do_div(temp_clk, pwm_clk);
	result->period = (unsigned int)temp_clk;
	temp_clk = cap_time[1] * 1000 * pwm_div;
	do_div(temp_clk, pwm_clk);
	result->duty_cycle = (unsigned int)temp_clk;

	reg_shift = index;
	/* enable pwm channel */
	value = sunxi_pwm_readl(pwm_chip, pc->data->per_offset);
	/*
	 * 0 , 1 --> 0
	 * 2 , 3 --> 2
	 * 4 , 5 --> 4
	 * 6 , 7 --> 6
	 */
	reg_shift &= ~(1);
	if (GET_BITS(reg_shift, 2, value) == 0) {
		/* Pwm channel with capture function enabled */
		value = sunxi_pwm_readl(pwm_chip, pc->data->cer_offset);
		if (GET_BITS(reg_shift, 2, value) == 0) {
			/* disable clk for pwm controller. */
			err = get_pccr_reg_offset(index, &reg_offset);
			if (err) {
				dev_err(pc->pwm_chip.dev, "get pwm channel failed\n");
				return -EINVAL;
			}

			reg_shift = PWM_CLK_GATING_SHIFT;
			value = sunxi_pwm_readl(pwm_chip, reg_offset);
			value = SET_BITS(reg_shift, 0x1, value, 0);
			sunxi_pwm_writel(pwm_chip, reg_offset, value);
		}
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);

	if (timeout <= 0) {
		pr_err("%s: pwm capture timeout !\n", __func__);
		return -1;
	}
	return 0;
}

static int sunxi_pwm_get_state(struct pwm_chip *pwm_chip,
				struct pwm_device *pwm_device,
				struct pwm_state *state)
{
	unsigned int reg_offset;
	u32 val, sel;
	struct sunxi_pwm_chip *pc;

	pc = to_sunxi_pwm_chip(pwm_chip);

	/* Read Polarity */
	sel = pwm_device->pwm - pwm_chip->base;
	reg_offset = pc->data->pcr_base_offset + sel * PWM_REG_UNIFORM_OFFSET;

	val = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (val & BIT_MASK(8))
		state->polarity = PWM_POLARITY_NORMAL;
	else
		state->polarity = PWM_POLARITY_INVERSED;

	/* Read Enable */
	reg_offset = pc->data->per_offset;
	val = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (val & BIT_MASK(sel)) {
		state->enabled = true;
	}
	else {
		state -> enabled = false;
	}

	/*
	 * FIXME: Duty Cycle and Period not implemented since I don't want to try to decode clksrc stuff
	 * Blame allwinner for not implementing it in the BSP in the first place
	 */

	return 0;

}
static int sunxi_pwm_resource_get(struct platform_device *pdev,
				  struct sunxi_pwm_chip *pc,
				  struct device_node *np)
{
	struct resource *res;
	int err;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "fail to get pwm IORESOURCE_MEM\n");
		return -EINVAL;
	}

	pc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pc->base)) {
		dev_err(&pdev->dev, "fail to map pwm IO resource\n");
		return PTR_ERR(pc->base);
	}

	pc->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(pc->reset)) {
		dev_err(&pdev->dev, "%s: can't get pwm reset clk\n", __func__);
		return PTR_ERR(pc->reset);
	}

	pc->clk = devm_clk_get(&pdev->dev, NULL);
	if (!pc->clk) {
		dev_err(&pdev->dev, "fail to get pwm clk!\n");
		return -EINVAL;
	}

	/* read property pwm-number */
	err = of_property_read_u32(np, "pwm-number", &pc->pwm_chip.npwm);
	if (err) {
		dev_err(&pdev->dev, "failed to get pwm number!\n");
		return -EINVAL;
	}

	/* read property pwm-base */
	err = of_property_read_u32(np, "pwm-base", &pc->pwm_chip.base);
	if (err) {
		dev_err(&pdev->dev, "failed to get pwm-base!\n");
		return -EINVAL;
	}

	err = of_property_read_u32(np, "#pwm-cells", &pc->cells_num);
	if (err) {
		dev_err(&pdev->dev, "failed to get pwm-cells!\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_pwm_hw_init(struct platform_device *pdev,
			     struct sunxi_pwm_chip *pc)
{
	int err;

	err = reset_control_deassert(pc->reset);
	if (err) {
		dev_err(&pdev->dev, "deassert pwm reset failed\n");
		return err;
	}

	err = clk_prepare_enable(pc->clk);
	if (err) {
		dev_err(&pdev->dev, "try to enbale pwm clk failed\n");
		goto assert_reset;
	}

	return 0;

assert_reset:
	reset_control_assert(pc->reset);

	return err;
}

static void sunxi_pwm_hw_exit(struct sunxi_pwm_chip *pc)
{
	clk_disable_unprepare(pc->clk);

	reset_control_assert(pc->reset);
}

static int sunxi_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			   const struct pwm_state *state)
{
	// This is taken from the old driver/pwm/core.c to use the deprecated PWM functions that aren't atomic
	// Yeah this *technically* isn't atomic but there are bigger issues with Allwinner's BSP code than that
	// I just want a working backlight man...

	int err;

	if (!pwm || !state || !state->period ||
	    state->duty_cycle > state->period)
		return -EINVAL;


	if (state->period == pwm->state.period &&
	    state->duty_cycle == pwm->state.duty_cycle &&
	    state->polarity == pwm->state.polarity &&
	    state->enabled == pwm->state.enabled)
		return 0;

	/*
	 * FIXME: restore the initial state in case of error.
	 */
	if (state->polarity != pwm->state.polarity) {
		/*
		 * Changing the polarity of a running PWM is
		 * only allowed when the PWM driver implements
		 * ->apply().
		 */
		if (pwm->state.enabled) {
			sunxi_pwm_disable(chip, pwm);
			pwm->state.enabled = false;
		}

		err = sunxi_pwm_set_polarity(chip, pwm,
							state->polarity);
		if (err)
			return err;

		pwm->state.polarity = state->polarity;
	}

	if (state->period != pwm->state.period ||
		state->duty_cycle != pwm->state.duty_cycle) {
		err = sunxi_pwm_config_channel(chip, pwm,
					state->duty_cycle,
					state->period);
		if (err)
			return err;

		pwm->state.duty_cycle = state->duty_cycle;
		pwm->state.period = state->period;
	}

	if (state->enabled != pwm->state.enabled) {
		if (state->enabled) {
			err = sunxi_pwm_enable(chip, pwm);
			if (err)
				return err;
		} else {
			sunxi_pwm_disable(chip, pwm);
		}

		pwm->state.enabled = state->enabled;
	}

	return 0;
}

static struct pwm_ops sunxi_pwm_ops = {
	.apply = sunxi_pwm_apply,
	.capture = sunxi_pwm_capture,
	.get_state = sunxi_pwm_get_state,
	.owner = THIS_MODULE,
};

static const struct of_device_id sunxi_pwm_match[] = {
	{ .compatible = "allwinner,sunxi-pwm",		.data = &sunxi_pwm_v200_data},
	{ .compatible = "allwinner,sunxi-s_pwm",	.data = &sunxi_pwm_v200_data},
	{ .compatible = "allwinner,sunxi-pwm-v100",	.data = &sunxi_pwm_v100_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_pwm_match);

static int sunxi_pwm_fill_hw_data(struct sunxi_pwm_chip *pc)
{
	size_t size;
	const struct of_device_id *of_id;

	/* get hw data from match table */
	of_id = of_match_device(sunxi_pwm_match, pc->pwm_chip.dev);
	if (!of_id) {
		dev_err(pc->pwm_chip.dev, "of_match_device() failed\n");
		return -EINVAL;
	}

	pc->data = (struct sunxi_pwm_hw_data *)(of_id->data);

	size = sizeof(u32) * pc->data->pm_regs_num;
	pc->pm_regs_offset = devm_kzalloc(pc->pwm_chip.dev, size, GFP_KERNEL);
	pc->regs_backup = devm_kzalloc(pc->pwm_chip.dev, size, GFP_KERNEL);

	/* Configure the registers that need to be saved for wake-up from sleep */
	pc->pm_regs_offset[0] = PWM_PIER;
	pc->pm_regs_offset[1] = PWM_CIER;
	pc->pm_regs_offset[2] = pc->data->per_offset;
	pc->pm_regs_offset[3] = pc->data->cer_offset;
	if (pc->data->clk_gating_separate)
		pc->pm_regs_offset[4] = PWM_PCGR;

	return 0;
}

static int sunxi_pwm_probe(struct platform_device *pdev)
{
	int err;
	int i;
	struct sunxi_pwm_chip *pc;
	struct platform_device *pwm_pdevice;
	struct device_node *sub_np;
	struct device_node *np = pdev->dev.of_node;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	platform_set_drvdata(pdev, pc);
	pc->pwm_chip.dev = &pdev->dev;

	err = sunxi_pwm_fill_hw_data(pc);
	if (err) {
		dev_err(&pdev->dev, "unable to get hw_data\n");
		return err;
	}

	err = sunxi_pwm_resource_get(pdev, pc, np);
	if (err) {
		dev_err(&pdev->dev, "pwm failed to get resource\n");
		goto err0;
	}

	err = sunxi_pwm_hw_init(pdev, pc);
	if (err) {
		dev_err(&pdev->dev, "pwm failed to hw_init");
		goto err0;
	}

	pc->pwm_chip.dev = &pdev->dev;
	pc->pwm_chip.ops = &sunxi_pwm_ops;
	pc->pwm_chip.of_xlate = of_pwm_xlate_with_flags;
	pc->pwm_chip.of_pwm_n_cells = pc->cells_num;

	/* add pwm chip to pwm-core */
	err = pwmchip_add(&pc->pwm_chip);
	if (err < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", err);
		goto err1;
	}

	pc->config = devm_kzalloc(&pdev->dev, sizeof(*pc->config) * pc->pwm_chip.npwm, GFP_KERNEL);
	if (!pc->config) {
		err = -ENOMEM;
		goto err2;
	}

	for (i = 0; i < pc->pwm_chip.npwm; i++) {
		sub_np = of_parse_phandle(np, "sunxi-pwms", i);
		if (!sub_np) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			goto err2;
		}

		pwm_pdevice = of_find_device_by_node(sub_np);
		/* it may be the program is error or the status of pwm%d  is disabled */
		if (!pwm_pdevice) {
			pr_debug("%s:fail to find device for pwm%d, continue!\n", __func__, i);
			continue;
		}
		err = sunxi_pwm_get_config(pwm_pdevice, &pc->config[i]);
		if (err) {
			pr_err("Get config failed,exit!\n");
			goto err2;
		}
	}

	return 0;

err2:
	pwmchip_remove(&pc->pwm_chip);
err1:
	sunxi_pwm_hw_exit(pc);
err0:
	return err;
}

static int sunxi_pwm_remove(struct platform_device *pdev)
{
	struct sunxi_pwm_chip *pc;

	pc = platform_get_drvdata(pdev);

	pwmchip_remove(&pc->pwm_chip);
	sunxi_pwm_hw_exit(pc);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)

static void sunxi_pwm_stop_work(struct sunxi_pwm_chip *pc)
{
	int i;
	bool pwm_state;

	for (i = 0; i < pc->pwm_chip.npwm; i++) {
		pwm_state = pc->pwm_chip.pwms[i].state.enabled;
		pwm_disable(&pc->pwm_chip.pwms[i]);
		pc->pwm_chip.pwms[i].state.enabled = pwm_state;
	}
}

static void sunxi_pwm_start_work(struct sunxi_pwm_chip *pc)
{
	int i;
	struct pwm_state state;

	for (i = 0; i < pc->pwm_chip.npwm; i++) {
		pwm_get_state(&pc->pwm_chip.pwms[i], &state);
		pc->pwm_chip.pwms[i].state.period = 0;
		pc->pwm_chip.pwms[i].state.duty_cycle = 0;
		pc->pwm_chip.pwms[i].state.polarity = PWM_POLARITY_NORMAL;
		pwm_apply_state(&pc->pwm_chip.pwms[i], &state);
		if (pwm_is_enabled(&pc->pwm_chip.pwms[i])) {
			pc->pwm_chip.pwms[i].state.enabled = false;
			pwm_enable(&pc->pwm_chip.pwms[i]);
		}
	}
}

static int sunxi_pwm_suspend(struct device *dev)
{
	struct platform_device *pdev;
	struct sunxi_pwm_chip *pc;

	pdev = container_of(dev, struct platform_device, dev);
	pc = platform_get_drvdata(pdev);

	sunxi_pwm_stop_work(pc);

	sunxi_pwm_save_regs(pc);

	clk_disable_unprepare(pc->clk);

	reset_control_assert(pc->reset);

	return 0;
}

static int sunxi_pwm_resume(struct device *dev)
{
	struct platform_device *pdev;
	struct sunxi_pwm_chip *pc;
	int err;

	pdev = container_of(dev, struct platform_device, dev);
	pc = platform_get_drvdata(pdev);

	err = reset_control_deassert(pc->reset);
	if (err) {
		pr_err("reset_control_deassert() failed\n");
		return 0;
	}

	err = clk_prepare_enable(pc->clk);
	if (err) {
		pr_err("clk_prepare_enable() failed\n");
		return 0;
	}

	sunxi_pwm_restore_regs(pc);

	sunxi_pwm_start_work(pc);

	return 0;
}

static const struct dev_pm_ops pwm_pm_ops = {
	.suspend_late = sunxi_pwm_suspend,
	.resume_early = sunxi_pwm_resume,
};
#else
static const struct dev_pm_ops pwm_pm_ops;
#endif

static struct platform_driver sunxi_pwm_driver = {
	.probe = sunxi_pwm_probe,
	.remove = sunxi_pwm_remove,
	.driver = {
		.name = "sunxi_pwm",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_pwm_match,
		.pm = &pwm_pm_ops,
	 },
};

static int __init pwm_module_init(void)
{
	int ret = 0;

	pr_info("pwm module init!\n");

	if (ret == 0) {
		ret = platform_driver_register(&sunxi_pwm_driver);
	}

	return ret;
}

static void __exit pwm_module_exit(void)
{
	pr_info("pwm module exit!\n");

	platform_driver_unregister(&sunxi_pwm_driver);
}

subsys_initcall_sync(pwm_module_init);
module_exit(pwm_module_exit);

MODULE_AUTHOR("lihuaxing");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pwm");
MODULE_VERSION("1.1.0");
