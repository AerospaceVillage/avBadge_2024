/*
 * Allwinner SoCs SRAM Controller Driver
 *
 * Copyright (C) 2015 Maxime Ripard
 *
 * Author: Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#include <linux/soc/sunxi/sunxi_sram.h>
#include <linux/firmware/sunxi_dsp.h>
#include "sunxi_dsp_msgbox.h"
#include "sunxi_dsp_reg.h"
#include "sunxi_dsp_ihex.h"

static int matches_driver(struct device_driver *driver);

static inline void reg_bits_set(void __iomem *reg, u32 mask, u32 shift)
{
	u32 val;

	val = readl(reg);
	val |= (mask << shift);
	writel(val, reg);
}

static inline void reg_bits_clear(void __iomem *reg, u32 mask, u32 shift)
{
	u32 val;

	val = readl(reg);
	val &= ~(mask << shift);
	writel(val, reg);
}

static inline u32 reg_bits_get(void __iomem *reg, u32 mask, u32 shift)
{
	return (readl(reg) & (mask << shift)) >> shift;
}

static inline void reg_val_update(void __iomem *reg, u32 mask, u32 shift,
				  u32 val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(mask << shift);
	reg_val |= ((val & mask) << shift);
	writel(reg_val, reg);
}

struct sunxi_dsp_inst {
	struct mutex mtx;

	struct clk *clk_dsp;
	struct clk *clk_bus;
	struct clk *clk_cpu_msgbox;
	struct clk *clk_dsp_msgbox;
	struct reset_control *rst_dsp;
	struct reset_control *rst_cfg;
	struct reset_control *rst_dbg;
	struct reset_control *rst_cpu_msgbox;
	struct reset_control *rst_dsp_msgbox;
	void __iomem *reg_cfg;
	void __iomem *map_iram;
	void __iomem *map_dram0;
	void __iomem *map_dram1;
	void __iomem *reg_cpu_msgbox;
	void __iomem *reg_dsp_msgbox;
	struct pinctrl *pctrl;
	unsigned int dsp_speed;
	int irq_pfe;
	int irq_cpu_msgbox;
	mailbox_cb_t mbox_cb;
	void *mbox_cb_arg;
	bool enabled;

	dma_addr_t sample_buf_dma_addr;
	void *sample_buf;
};

void *sunxi_dsp_sample_buf_addr(struct platform_device *pdev)
{
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	return dsp->sample_buf;
}
EXPORT_SYMBOL_GPL(sunxi_dsp_sample_buf_addr);

void __iomem *sunxi_dsp_lookup_localaddr(struct platform_device *pdev, u32 addr) {
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	if (addr >= 0x00420000 && addr < 0x00428000) {
		addr -= 0x00420000;
		return dsp->map_dram0 + addr;
	}
	else if (addr >= 0x00440000 && addr < 0x00448000) {
		addr -= 0x00440000;
		return dsp->map_dram0 + addr;
	}
	else {
		return ERR_PTR(-EINVAL);
	}
}
EXPORT_SYMBOL_GPL(sunxi_dsp_lookup_localaddr);

/******************************************
 * DSP Message Box
 *****************************************/

static void sunxi_dsp_process_cpu_msgbox(struct platform_device *pdev)
{
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	unsigned int i;
	u32 irq_status;

	/* Read which msgboxes have outstanding messages */
	irq_status = readl(dsp->reg_cpu_msgbox +
			   SUNXI_MSGBOX_READ_IRQ_STATUS(SUNXI_MSGBOX_DSP));

	for (i = 0; i < SUNXI_MSGBOX_CHANNELS_MAX; i++) {
		/* Only process channel if IRQ pending */
		if (!((irq_status >> RD_IRQ_PEND_SHIFT(i)) &
		      RD_IRQ_PEND_MASK)) {
			continue;
		}

		unsigned int fifo_lvl;

		do {
			fifo_lvl = reg_bits_get(
				dsp->reg_cpu_msgbox +
					SUNXI_MSGBOX_MSG_STATUS(
						SUNXI_MSGBOX_DSP, i),
				MSG_NUM_MASK, MSG_NUM_SHIFT);

			if (fifo_lvl > 0) {
				u32 fdata;

				fdata = readl(dsp->reg_cpu_msgbox +
					      SUNXI_MSGBOX_MSG_FIFO(
						      SUNXI_MSGBOX_DSP, i));

				if (dsp->mbox_cb) {
					dsp->mbox_cb(i, fdata,
						     dsp->mbox_cb_arg);
				}

				dev_vdbg(&pdev->dev,
					 "Message on FIFO %d: 0x%08X\n", i,
					 fdata);
			}
		} while (fifo_lvl > 0);

		/*
		 * Clear the pending bit for this interrupt
		 * This must be done after reading, as this bit can't be
		 * cleared when there are outstanding messages.
		 */
		reg_bits_set(dsp->reg_cpu_msgbox + SUNXI_MSGBOX_READ_IRQ_STATUS(
							   SUNXI_MSGBOX_DSP),
			     RD_IRQ_PEND_MASK, RD_IRQ_PEND_SHIFT(i));
	}
}

static irqreturn_t sunxi_dsp_cpu_msgbox_irq(int irq, void *data)
{
	struct platform_device *pdev = data;
	sunxi_dsp_process_cpu_msgbox(pdev);

	return IRQ_HANDLED;
}

int sunxi_dsp_msgbox_send(struct platform_device *pdev, unsigned int p,
			  unsigned int val)
{
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	unsigned int fifo_lvl;

	fifo_lvl =
		reg_bits_get(dsp->reg_dsp_msgbox + SUNXI_MSGBOX_MSG_STATUS(
							   SUNXI_MSGBOX_DSP, p),
			     MSG_NUM_MASK, MSG_NUM_SHIFT);

	if (fifo_lvl <= 7) {
		writel(val, dsp->reg_dsp_msgbox +
				    SUNXI_MSGBOX_MSG_FIFO(SUNXI_MSGBOX_DSP, p));
		return 0;
	} else {
		/* FIFO Full */
		return -EAGAIN;
	}
}
EXPORT_SYMBOL_GPL(sunxi_dsp_msgbox_send);

/******************************************
 * DSP Enable Sequencing
 *****************************************/

struct sunxi_dsp_meminfo {
	u32 base_addr;
	u32 size;
};

/* Addresses from DSP perspective */
static const struct sunxi_dsp_meminfo iram_info = { .base_addr = 0x00400000,
						    .size = 64 * 1024 };

static const struct sunxi_dsp_meminfo dram0_info = { .base_addr = 0x00420000,
						     .size = 32 * 1024 };

static const struct sunxi_dsp_meminfo dram1_info = { .base_addr = 0x00440000,
						     .size = 32 * 1024 };

static void sunxi_dsp_set_runstall(struct sunxi_dsp_inst *dsp, u32 value)
{
	u32 reg_val;

	reg_val = readl(dsp->reg_cfg + DSP_CTRL_REG0);
	reg_val &= ~(1 << BIT_RUN_STALL);
	reg_val |= (value << BIT_RUN_STALL);
	writel(reg_val, (dsp->reg_cfg + DSP_CTRL_REG0));
}

static irqreturn_t sunxi_dsp_pfe_irq(int irq, void *data)
{
	struct platform_device *pdev = data;
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	u32 reg_val;

	reg_val = readl(dsp->reg_cfg + DSP_STAT_REG);
	if (reg_val & (1 << BIT_DEBUG_MODE)) {
		dev_err(&pdev->dev, "DSP entered debug mode\n");
	} else {
		dev_err(&pdev->dev, "DSP core crashed with fatal error\n");
	}
	disable_irq_nosync(irq);

	return IRQ_NONE;
}

static int sunxi_dsp_handle_start_address(ihex_decode_cfg_t *cfg, u32 addr)
{
	struct platform_device *pdev = cfg->arg;
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	u32 reg_val;

	// Only allow a vector inside of IRAM (that's the only thing we load)
	if (addr < iram_info.base_addr &&
	    addr - iram_info.base_addr >= iram_info.size) {
		cfg->error_msg = "Invalid Start Address";
		return -EINVAL;
	}

	// Set the alternate reset vector address
	writel(addr, dsp->reg_cfg + DSP_ALT_RESET_VEC_REG);

	// Set the core to boot from the alternate reset address
	reg_val = readl(dsp->reg_cfg + DSP_CTRL_REG0);
	reg_val |= (1 << BIT_START_VEC_SEL);
	writel(reg_val, dsp->reg_cfg + DSP_CTRL_REG0);

	return 0;
}

static int sunxi_dsp_handle_data_record(ihex_decode_cfg_t *cfg, u32 addr,
					unsigned char *data, size_t len)
{
	struct platform_device *pdev = cfg->arg;
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);

	// Load data record into the proper I/O mem space
	if (addr >= iram_info.base_addr &&
	    addr + len - iram_info.base_addr <= iram_info.size) {
		u32 offset = addr - iram_info.base_addr;
		memcpy_toio(dsp->map_iram + offset, data, len);
	} else if (addr >= dram0_info.base_addr &&
		   addr + len - dram0_info.base_addr <= dram0_info.size) {
		u32 offset = addr - dram0_info.base_addr;
		memcpy_toio(dsp->map_dram0 + offset, data, len);
	} else if (addr >= dram1_info.base_addr &&
		   addr + len - dram1_info.base_addr <= dram1_info.size) {
		u32 offset = addr - dram1_info.base_addr;
		memcpy_toio(dsp->map_dram1 + offset, data, len);
	} else {
		cfg->error_msg = "Invalid Destination Address";
		return -EINVAL;
	}

	return 0;
}

int sunxi_dsp_enable(const char *fw_name, struct platform_device *pdev,
		     mailbox_cb_t cb, void *cb_arg)
{
	ihex_decode_cfg_t cfg = {};
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	const struct firmware *fw;
	u32 reg_val;
	int ret;

	mutex_lock(&dsp->mtx);
	if (dsp->enabled) {
		dev_warn(&pdev->dev, "can't enable: dsp already enabled\n");
		ret = -EBUSY;
		goto spinlock_release;
	}

	/* Grab the firmware image */
	ret = request_firmware(&fw, fw_name, &pdev->dev);
	if (ret) {
		dev_warn(&pdev->dev, "failed to load firmware '%s': %d\n",
			 fw_name, ret);
		goto spinlock_release;
	}

	/* Map SRAM to local memory for configuration */
	ret = sunxi_sram_remap_set(SUNXI_SRAM_REMAP_LOCAL);
	if (ret) {
		dev_warn(&pdev->dev, "failed to remap DSP SRAM to local\n");
		goto free_firmware;
	}

	/* First enable DSP Module clocks at the correct speed */
	ret = clk_set_rate_exclusive(dsp->clk_dsp, dsp->dsp_speed);
	if (ret) {
		dev_warn(&pdev->dev, "failed to set dsp clock to %d: %d\n",
			 dsp->dsp_speed, ret);
		goto free_firmware;
	}

	ret = clk_prepare_enable(dsp->clk_dsp);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to set prepare/enable dsp clk: %d\n", ret);
		goto cleanup_clk_rate;
	}

	/* Enable bus clocks to the control peripheral & msgboxes */
	ret = clk_prepare_enable(dsp->clk_bus);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to set prepare/enable bus clk: %d\n", ret);
		goto cleanup_mod_clk;
	}

	ret = clk_prepare_enable(dsp->clk_cpu_msgbox);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to set prepare/enable cpu msgbox clk: %d\n",
			 ret);
		goto cleanup_bus_clk;
	}

	ret = clk_prepare_enable(dsp->clk_dsp_msgbox);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to set prepare/enable dsp msgbox clk: %d\n",
			 ret);
		goto cleanup_cpu_msgbox_clk;
	}

	/* Take cfg/msgbox/debug hardware out of reset */
	ret = reset_control_deassert(dsp->rst_cfg);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to enable dsp config peripheral: %d\n", ret);
		goto cleanup_dsp_msgbox_clk;
	}

	ret = reset_control_deassert(dsp->rst_cpu_msgbox);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to enable cpu msgbox peripheral: %d\n", ret);
		goto cleanup_cfg_rst;
	}

	ret = reset_control_deassert(dsp->rst_dsp_msgbox);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to enable dsp msgbox peripheral: %d\n", ret);
		goto cleanup_cpu_msgbox_rst;
	}

	ret = reset_control_deassert(dsp->rst_dbg);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to enable dsp debug peripheral: %d\n", ret);
		goto cleanup_dsp_msgbox_rst;
	}

	/* set DSP to stall to halt execution until program load done */
	sunxi_dsp_set_runstall(dsp, DSP_CTRL_STALL);

	/* set dsp clken and default reset vector (ihex can override though) */
	reg_val = readl(dsp->reg_cfg + DSP_CTRL_REG0);
	reg_val |= (1 << BIT_DSP_CLKEN);
	reg_val &= ~(1 << BIT_START_VEC_SEL);
	writel(reg_val, dsp->reg_cfg + DSP_CTRL_REG0);

	/* Take DSP out of reset. It will halt until STALL bit is cleared */
	ret = reset_control_deassert(dsp->rst_dsp);
	if (ret) {
		dev_warn(&pdev->dev, "failed to enable dsp: %d\n", ret);
		goto cleanup_dbg_rst;
	}

	/* Swap SRAM over to DSP before program load
	 * since DRAM has a different striping in local mode */
	ret = sunxi_sram_remap_set(SUNXI_SRAM_REMAP_DSP);
	if (ret) {
		dev_warn(&pdev->dev, "failed to remap DSP SRAM to DSP\n");
		goto cleanup_dsp_rst;
	}

	/* Load in the firmware from the ihex file */
	/* This will load the firmware into DSP SRAM and set the start vector */
	cfg.handle_data_record = &sunxi_dsp_handle_data_record;
	cfg.handle_start_addr_record = &sunxi_dsp_handle_start_address;
	cfg.arg = pdev;

	ret = decode_ihex(&cfg, fw->data, fw->size);
	if (ret) {
		if (cfg.error_msg) {
			dev_warn(
				&pdev->dev,
				"failed to load dsp firmware ihex (%d) - Record %d: %s\n",
				ret, cfg.decoded_record_count, cfg.error_msg);
			goto cleanup_dbg_rst;
		} else {
			dev_warn(&pdev->dev,
				 "failed to load dsp firmware ihex (%d)\n",
				 ret);
			goto cleanup_dbg_rst;
		}
	}

	/* Reset DSP one more time since the ihex load might have
	   changed the reset vector (which is only sampled on reset) */
	ret = reset_control_assert(dsp->rst_dsp);
	if (ret) {
		dev_warn(&pdev->dev, "failed to reset dsp: %d\n", ret);
		goto cleanup_dbg_rst;
	}
	ret = reset_control_deassert(dsp->rst_dsp);
	if (ret) {
		dev_warn(&pdev->dev, "failed to take dsp out of reset: %d\n",
			 ret);
		goto cleanup_dbg_rst;
	}

	/* Initialize Message Box */
	/* Enable all interrupts */
	writel(0x55, dsp->reg_cpu_msgbox +
			     SUNXI_MSGBOX_READ_IRQ_ENABLE(SUNXI_MSGBOX_DSP));
	sunxi_dsp_process_cpu_msgbox(pdev);

	/* Finally good to start the processor */
	sunxi_dsp_set_runstall(dsp, DSP_CTRL_RUN);

	/* Request the Fatal Error IRQ */
	ret = devm_request_irq(&pdev->dev, dsp->irq_pfe, &sunxi_dsp_pfe_irq, 0,
			       "dsp_pfeirq", pdev);
	if (ret) {
		dev_warn(&pdev->dev, "failed to request DSP PFE irq\n");
		goto cleanup_dsp_rst;
	}

	/* Request the CPU Msgbox IRQ */
	ret = devm_request_irq(&pdev->dev, dsp->irq_cpu_msgbox,
			       &sunxi_dsp_cpu_msgbox_irq, 0, "cpu_msgbox_r",
			       pdev);
	if (ret) {
		dev_warn(&pdev->dev, "failed to request cpu msgbox irq\n");
		goto cleanup_pfe_irq;
	}

	/* Allocate the DMA buffer */
	dsp->sample_buf = dma_alloc_coherent(&pdev->dev, SAMPLE_BUF_LEN,
					     &dsp->sample_buf_dma_addr,
					     GFP_KERNEL);
	if (!dsp->sample_buf) {
		dev_warn(&pdev->dev, "dma_alloc_coherent fail, size=0x%x\n",
			 SAMPLE_BUF_LEN);

		ret = -ENOMEM;
		goto cleanup_msgbox_irq;
	}

	ret = sunxi_dsp_msgbox_send(pdev, 0, (u32)dsp->sample_buf_dma_addr);
	if (ret) {
		dev_warn(&pdev->dev, "failed to send DMA length\n");
		goto cleanup_dma_alloc;
	}

	dev_info(&pdev->dev, "booting dsp (Speed: %d Hz, firmware: '%s')\n",
		 dsp->dsp_speed, fw_name);

	dsp->enabled = true;
	dsp->mbox_cb = cb;
	dsp->mbox_cb_arg = cb_arg;

	release_firmware(fw);
	mutex_unlock(&dsp->mtx);
	return ret;

cleanup_dma_alloc:
	dma_free_coherent(&pdev->dev, SAMPLE_BUF_LEN, dsp->sample_buf,
			  dsp->sample_buf_dma_addr);
	dsp->sample_buf = NULL;
	dsp->sample_buf_dma_addr = 0;

cleanup_msgbox_irq:
	devm_free_irq(&pdev->dev, dsp->irq_cpu_msgbox, pdev);

cleanup_pfe_irq:
	devm_free_irq(&pdev->dev, dsp->irq_pfe, pdev);

cleanup_dsp_rst:
	reset_control_assert(dsp->rst_dsp);

cleanup_dbg_rst:
	reset_control_assert(dsp->rst_dbg);

cleanup_dsp_msgbox_rst:
	reset_control_assert(dsp->rst_dsp_msgbox);

cleanup_cpu_msgbox_rst:
	reset_control_assert(dsp->rst_cpu_msgbox);

cleanup_cfg_rst:
	reset_control_assert(dsp->rst_cfg);

cleanup_dsp_msgbox_clk:
	clk_disable_unprepare(dsp->clk_dsp_msgbox);

cleanup_cpu_msgbox_clk:
	clk_disable_unprepare(dsp->clk_cpu_msgbox);

cleanup_bus_clk:
	clk_disable_unprepare(dsp->clk_bus);

cleanup_mod_clk:
	clk_disable_unprepare(dsp->clk_dsp);

cleanup_clk_rate:
	clk_rate_exclusive_put(dsp->clk_dsp);

free_firmware:
	release_firmware(fw);

spinlock_release:
	mutex_unlock(&dsp->mtx);

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_dsp_enable);

void sunxi_dsp_disable(struct platform_device *pdev)
{
	struct sunxi_dsp_inst *dsp = platform_get_drvdata(pdev);
	int ret;

	mutex_lock(&dsp->mtx);
	if (!dsp->enabled) {
		mutex_unlock(&dsp->mtx);
		return;
	}

	dev_info(&pdev->dev, "Disabling DSP\n");
	dsp->mbox_cb = NULL;
	dsp->mbox_cb_arg = NULL;

	devm_free_irq(&pdev->dev, dsp->irq_cpu_msgbox, pdev);
	devm_free_irq(&pdev->dev, dsp->irq_pfe, pdev);

	ret = reset_control_assert(dsp->rst_dsp);
	if (ret) {
		dev_warn(&pdev->dev, "failed to reset dsp core: %d\n", ret);
	}

	ret = reset_control_assert(dsp->rst_dbg);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to reset dsp debug peripheral: %d\n", ret);
	}

	ret = reset_control_assert(dsp->rst_dsp_msgbox);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to reset dsp msgbox peripheral: %d\n", ret);
	}

	ret = reset_control_assert(dsp->rst_cpu_msgbox);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to reset cpu msgbox peripheral: %d\n", ret);
	}

	ret = reset_control_assert(dsp->rst_cfg);
	if (ret) {
		dev_warn(&pdev->dev,
			 "failed to reset dsp config peripheral: %d\n", ret);
	}

	clk_disable_unprepare(dsp->clk_dsp_msgbox);
	clk_disable_unprepare(dsp->clk_cpu_msgbox);
	clk_disable_unprepare(dsp->clk_bus);
	clk_disable_unprepare(dsp->clk_dsp);
	clk_rate_exclusive_put(dsp->clk_dsp);

	ret = sunxi_sram_remap_set(SUNXI_SRAM_REMAP_LOCAL);
	if (ret) {
		dev_warn(&pdev->dev, "failed to remap DSP SRAM to local: %d\n",
			 ret);
	}

	dma_free_coherent(&pdev->dev, SAMPLE_BUF_LEN, dsp->sample_buf,
			  dsp->sample_buf_dma_addr);
	dsp->sample_buf = NULL;
	dsp->sample_buf_dma_addr = 0;

	dsp->enabled = false;

	mutex_unlock(&dsp->mtx);
}
EXPORT_SYMBOL_GPL(sunxi_dsp_disable);

struct platform_device *sunxi_dsp_by_phandle(struct device *dev, int index)
{
	struct device_node *node, *np = dev_of_node(dev);

	if (!np) {
		dev_dbg(dev, "device does not have a device node entry\n");
		return ERR_PTR(-EINVAL);
	}

	node = of_parse_phandle(np, "dsp-inst", index);
	if (!node) {
		dev_dbg(dev, "failed to get phandle in %pOF node\n", np);
		return ERR_PTR(-ENODEV);
	}

	if (!node->fwnode.dev) {
		dev_dbg(dev, "failed to access underlying fw device\n");
		return ERR_PTR(-EPROBE_DEFER);
	}

	if (!matches_driver(node->fwnode.dev->driver)) {
		dev_dbg(dev, "driver does not match: %p\n",
			node->fwnode.dev->driver);
		return ERR_PTR(-EPROBE_DEFER);
	}
	struct platform_device *dev_out = to_platform_device(node->fwnode.dev);

	of_node_put(node);

	return dev_out;
}
EXPORT_SYMBOL_GPL(sunxi_dsp_by_phandle);

/******************************************
 * Driver Bindings
 *****************************************/

static int sunxi_dsp_probe(struct platform_device *pdev)
{
	int err;
	struct sunxi_dsp_inst *pc;
	struct device_node *np = pdev->dev.of_node;

	/* Before doing anything else, try to get SRAM mapping
	 * If this fails, then we are still waiting on the SRAM driver to probe
	 */
	err = sunxi_sram_remap_get();
	if (err < 0)
		return err;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	platform_set_drvdata(pdev, pc);
	pc->enabled = false;
	pc->mbox_cb = NULL;
	mutex_init(&pc->mtx);

	dev_dbg(&pdev->dev, "Initializing dsp driver\n");

	/* Get all clocks for DSP control */
	pc->clk_dsp = devm_clk_get(&pdev->dev, "mod");
	if (IS_ERR(pc->clk_dsp))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->clk_dsp),
				     "can't get module clock\n");

	pc->clk_bus = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR(pc->clk_bus))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->clk_bus),
				     "can't get bus clock\n");

	pc->clk_dsp_msgbox = devm_clk_get(&pdev->dev, "dsp_msgbox");
	if (IS_ERR(pc->clk_dsp_msgbox))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->clk_dsp_msgbox),
				     "can't get dsp msgbox clock\n");

	pc->clk_cpu_msgbox = devm_clk_get(&pdev->dev, "cpu_msgbox");
	if (IS_ERR(pc->clk_cpu_msgbox))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->clk_cpu_msgbox),
				     "can't get cpu msgbox clock\n");

	/* Get all resets for DSP control */
	pc->rst_dsp = devm_reset_control_get_exclusive(&pdev->dev, "mod");
	if (IS_ERR(pc->rst_dsp))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->rst_dsp),
				     "can't get module reset control\n");

	pc->rst_cfg = devm_reset_control_get_exclusive(&pdev->dev, "cfg");
	if (IS_ERR(pc->rst_cfg))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->rst_cfg),
				     "can't get dsp config reset control\n");

	pc->rst_dbg = devm_reset_control_get_exclusive(&pdev->dev, "dbg");
	if (IS_ERR(pc->rst_dbg))
		return dev_err_probe(
			&pdev->dev, PTR_ERR(pc->rst_dbg),
			"can't get debug controller reset control\n");

	pc->rst_dsp_msgbox =
		devm_reset_control_get_exclusive(&pdev->dev, "dsp_msgbox");
	if (IS_ERR(pc->rst_dsp_msgbox))
		return dev_err_probe(
			&pdev->dev, PTR_ERR(pc->rst_dsp_msgbox),
			"can't get dsp msgbox config reset control\n");

	pc->rst_cpu_msgbox =
		devm_reset_control_get_exclusive(&pdev->dev, "cpu_msgbox");
	if (IS_ERR(pc->rst_cpu_msgbox))
		return dev_err_probe(
			&pdev->dev, PTR_ERR(pc->rst_cpu_msgbox),
			"can't get cpu msgbox config reset control\n");

	/* Load in the register maps */
	pc->reg_cfg = devm_platform_ioremap_resource_byname(pdev, "cfg");
	if (IS_ERR(pc->reg_cfg))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->reg_cfg),
				     "failed to init cfg mmio\n");

	pc->reg_dsp_msgbox =
		devm_platform_ioremap_resource_byname(pdev, "dsp_msgbox");
	if (IS_ERR(pc->reg_dsp_msgbox))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->reg_dsp_msgbox),
				     "failed to init dsp msgbox mmio\n");

	pc->reg_cpu_msgbox =
		devm_platform_ioremap_resource_byname(pdev, "cpu_msgbox");
	if (IS_ERR(pc->reg_cpu_msgbox))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->reg_cpu_msgbox),
				     "failed to init cpu msgbox mmio\n");

	pc->map_iram = devm_platform_ioremap_resource_byname(pdev, "iram");
	if (IS_ERR(pc->map_iram))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->map_iram),
				     "failed to init iram mmio\n");

	pc->map_dram0 = devm_platform_ioremap_resource_byname(pdev, "dram0");
	if (IS_ERR(pc->map_dram0))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->map_dram0),
				     "failed to init dram0 mmio\n");

	pc->map_dram1 = devm_platform_ioremap_resource_byname(pdev, "dram1");
	if (IS_ERR(pc->map_dram1))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->map_dram1),
				     "failed to init dram1 mmio\n");

	/* Load in pinctrl so JTAG can be optionally enabled */
	pc->pctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pc->pctrl))
		return dev_err_probe(&pdev->dev, PTR_ERR(pc->pctrl),
				     "failed to init pinctrl\n");

	/* Request dsp interrupts */
	pc->irq_pfe = platform_get_irq_byname(pdev, "pfe");
	if (pc->irq_pfe < 0)
		return dev_err_probe(&pdev->dev, pc->irq_pfe,
				     "failed to get pfe irq\n");

	pc->irq_cpu_msgbox = platform_get_irq_byname(pdev, "cpu_msgbox_r");
	if (pc->irq_cpu_msgbox < 0)
		return dev_err_probe(&pdev->dev, pc->irq_cpu_msgbox,
				     "failed to get cpu msgbox irq\n");

	/* Load device speed parameter */
	err = of_property_read_u32(np, "clock-frequency", &pc->dsp_speed);
	if (err) {
		return dev_err_probe(
			&pdev->dev, err,
			"can't read 'speed' device tree property\n");
	}

	return err;
}

static void sunxi_dsp_remove(struct platform_device *pdev)
{
	sunxi_dsp_disable(pdev);
}

static const struct of_device_id sunxi_dsp_dt_match[] = {
	{
		.compatible = "allwinner,sun20i-d1-dsp",
		.data = 0,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_dsp_dt_match);

static struct platform_driver sunxi_dsp_driver = {
	.probe = sunxi_dsp_probe,
	.remove_new = sunxi_dsp_remove,
	.driver = {
		.name		= "sunxi-dsp",
		.owner  = THIS_MODULE,
		.of_match_table	= sunxi_dsp_dt_match,
	},
};
module_platform_driver(sunxi_dsp_driver);

static int matches_driver(struct device_driver *driver)
{
	return driver == &sunxi_dsp_driver.driver;
}

MODULE_AUTHOR("Robert Pafford <19439938+rjp5th@users.noreply.github.com>");
MODULE_DESCRIPTION("Allwinner sunXi HiFi DSP Driver");
