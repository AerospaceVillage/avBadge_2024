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

#ifndef _SUNXI_SRAM_H_
#define _SUNXI_SRAM_H_

#define SUNXI_SRAM_REMAP_DSP		0
#define SUNXI_SRAM_REMAP_LOCAL		1

int sunxi_sram_claim(struct device *dev);
void sunxi_sram_release(struct device *dev);
int sunxi_sram_remap_set(int mapping);
// Returns 1 if locally mapped, 0 if dsp mapped, and negative on error
int sunxi_sram_remap_get(void);

#endif /* _SUNXI_SRAM_H_ */
