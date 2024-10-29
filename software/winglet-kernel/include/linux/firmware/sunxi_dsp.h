#ifndef __FIRMWARE_SUNXI_DSP_H
#define __FIRMWARE_SUNXI_DSP_H

#include <linux/io.h>
#include <linux/platform_device.h>

#define SAMPLE_BUF_LEN 0x400000

typedef void (*mailbox_cb_t)(unsigned int mbox, unsigned int val, void *arg);

int sunxi_dsp_enable(const char *fw_name, struct platform_device *pdev,
		     mailbox_cb_t cb, void *cb_arg);
void sunxi_dsp_disable(struct platform_device *pdev);
struct platform_device *sunxi_dsp_by_phandle(struct device *dev, int index);
int sunxi_dsp_msgbox_send(struct platform_device *pdev, unsigned int p,
				 unsigned int val);
void* sunxi_dsp_sample_buf_addr(struct platform_device *pdev);
void __iomem *sunxi_dsp_lookup_localaddr(struct platform_device *pdev, u32 addr);

#endif
