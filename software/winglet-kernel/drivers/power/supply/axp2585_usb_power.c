#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/gpio/consumer.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/err.h>
#include <linux/mfd/axp2101.h>
#include "axp2101_charger.h"

struct axp2585_usb_power {
	char                      *name;
	struct device             *dev;
	struct axp_config_info     dts_info;
	struct regmap             *regmap;
	struct power_supply       *usb_supply;
	struct delayed_work        usb_supply_mon;
	// struct delayed_work        usb_chg_state;

	// atomic_t set_current_limit;
};

// static int axp2585_usb_set_ihold(struct axp2585_usb_power *usb_power, int cur)
// {
// 	struct regmap *map = usb_power->regmap;
// 	u8 tmp;

// 	if (cur) {
// 		if (cur >= 100 && cur <= 3250) {
// 			tmp = (cur - 100)/50;
// 			regmap_update_bits(map, 0x10, 0x3f, tmp);
// 		}
// 	}

// 	return 0;
// }

// static void axp2585_usb_set_current_fsm(struct work_struct *work)
// {
// 	struct axp2585_usb_power *usb_power =
// 		container_of(work, typeof(*usb_power), usb_chg_state.work);
// 	struct axp_config_info *axp_config = &usb_power->dts_info;

// 	if (atomic_read(&usb_power->set_current_limit)) {
// 		pr_info("current limit setted: usb pc type\n");
// 	} else {
// 		axp2585_usb_set_ihold(usb_power, axp_config->pmu_usbad_cur);
// 		pr_info("current limit not set: usb adapter type\n");
// 	}
// }

static int axp2585_read_vbus_state(struct power_supply *ps,
					union power_supply_propval *val)
{
	struct axp2585_usb_power *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2585_STATUS, &data);
	if (ret < 0)
		return ret;

	val->intval = !!(data & (1 << 1));

	return ret;
}

static int axp2585_get_usb_ihold(struct power_supply *ps,
					union power_supply_propval *val)
{
	struct axp2585_usb_power *di = power_supply_get_drvdata(ps);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, 0x10, &data);
	if (ret < 0)
		return ret;

	val->intval = data * 50 + 100;

	return ret;
}

static enum power_supply_property axp2585_usb_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
};

static int axp2585_usb_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		axp2585_read_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		axp2585_get_usb_ihold(psy, val);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

// static int axp2585_usb_set_property(struct power_supply *psy,
// 				    enum power_supply_property psp,
// 				    const union power_supply_propval *val)
// {
// 	int ret = 0;
// 	struct axp2585_usb_power *usb_power = power_supply_get_drvdata(psy);

// 	switch (psp) {
// 	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
// 		ret = axp2585_usb_set_ihold(usb_power, val->intval);
// 		atomic_set(&usb_power->set_current_limit, 1);
// 		break;
// 	default:
// 		ret = -EINVAL;
// 	}

// 	return ret;
// }

// static int axp2585_usb_power_property_is_writeable(struct power_supply *psy,
// 		enum power_supply_property psp)
// {
// 	int ret;

// 	switch (psp) {
// 	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
// 		ret = 1;
// 		break;
// 	default:
// 		ret = 0;
// 	}

// 	return ret;
// }

static const struct power_supply_desc axp2585_usb_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = axp2585_usb_props,
	.num_properties = ARRAY_SIZE(axp2585_usb_props),
	.get_property = axp2585_usb_get_property,
	// .set_property = axp2585_usb_set_property,
	// .property_is_writeable = axp2585_usb_power_property_is_writeable,
};

static void axp2585_set_usb_vhold(struct regmap *regmap, u32 vol)
{
	u8 tmp;

	if (vol) {
		if (vol >= 3880 && vol <= 5080) {
			tmp = (vol - 3880)/80;
			regmap_update_bits(regmap, 0x11, 0x03, tmp);
		}
	}
}

static int axp2585_usb_power_init(struct axp2585_usb_power *usb_power)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;
	struct regmap *regmap = usb_power->regmap;

	axp2585_set_usb_vhold(regmap, axp_config->pmu_usbpc_vol);
	return 0;
}

static irqreturn_t axp2585_usb_power_in_irq(int irq, void *data)
{
	struct axp2585_usb_power *usb_power = data;
	// struct axp_config_info *axp_config = &usb_power->dts_info;

	power_supply_changed(usb_power->usb_supply);

	// axp2585_usb_set_ihold(usb_power, axp_config->pmu_usbpc_cur);
	// atomic_set(&usb_power->set_current_limit, 0);

	// cancel_delayed_work_sync(&usb_power->usb_chg_state);
	// schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(5 * 1000));

	return IRQ_HANDLED;
}

static irqreturn_t axp2585_usb_power_out_irq(int irq, void *data)
{
	struct axp2585_usb_power *usb_power = data;

	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

enum axp2585_usb_power_virqs {
	AXP2585_VIRQ_USBIN,
	AXP2585_VIRQ_USBRE,

	AXP2585_USB_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp2585_usb_irq[] = {
	[AXP2585_VIRQ_USBIN] = {"usb in", axp2585_usb_power_in_irq},
	[AXP2585_VIRQ_USBRE] = {"usb out", axp2585_usb_power_out_irq},
};

static void axp2585_usb_power_monitor(struct work_struct *work)
{
	struct axp2585_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
}

static int axp2585_usb_power_dt_parse(struct axp2585_usb_power *usb_power)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;
	struct device_node *node = usb_power->dev->of_node;

	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_usbpc_vol,                  4400);
	// AXP_OF_PROP_READ(pmu_usbpc_cur,                     0);
	// AXP_OF_PROP_READ(pmu_usbad_vol,                  4400);
	// AXP_OF_PROP_READ(pmu_usbad_cur,                     0);

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");

	return 0;
}

static int axp2585_usb_power_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct axp2585_usb_power *usb_power;
	int i, irq;
	int ret = 0;

	if (!axp_dev->irq) {
		pr_err("can not register axp2585 usb without irq\n");
		return -EINVAL;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (!usb_power) {
		pr_err("axp2585 usb power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	usb_power->name = "axp2585-usb-power";
	usb_power->dev = &pdev->dev;
	usb_power->regmap = axp_dev->regmap;

	platform_set_drvdata(pdev, usb_power);

	ret = axp2585_usb_power_dt_parse(usb_power);
	if (ret) {
		pr_err("%s parse device tree err\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = axp2585_usb_power_init(usb_power);
	if (ret < 0) {
		pr_err("axp2585 init usb power fail!\n");
		ret = -ENODEV;
		return ret;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	usb_power->usb_supply = devm_power_supply_register(usb_power->dev,
			&axp2585_usb_desc, &psy_cfg);

	if (IS_ERR(usb_power->usb_supply)) {
		pr_err("axp2585 failed to register usb power\n");
		ret = PTR_ERR(usb_power->usb_supply);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(axp2585_usb_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp2585_usb_irq[i].name);
		if (irq < 0) {
			dev_warn(&pdev->dev, "No IRQ for %s: %d\n",
				 axp2585_usb_irq[i].name, irq);
			continue;
		}
		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp2585_usb_irq[i].isr, 0,
						   axp2585_usb_irq[i].name, usb_power);
		if (ret < 0)
			dev_warn(&pdev->dev, "Error requesting %s IRQ %d: %d\n",
				axp2585_usb_irq[i].name, irq, ret);

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp2585_usb_irq[i].name, irq, ret);

		/* we use this variable to suspend irq */
		axp2585_usb_irq[i].irq = irq;
	}

	INIT_DELAYED_WORK(&usb_power->usb_supply_mon, axp2585_usb_power_monitor);
	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));

	// INIT_DELAYED_WORK(&usb_power->usb_chg_state, axp2585_usb_set_current_fsm);
	// schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(20 * 1000));

	return 0;
}

static int axp2585_usb_power_remove(struct platform_device *pdev)
{
	struct axp2585_usb_power *usb_power = platform_get_drvdata(pdev);

	if (usb_power->usb_supply)
		power_supply_unregister(usb_power->usb_supply);

	return 0;
}

static inline void axp_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2585_usb_virq_dts_set(struct axp2585_usb_power *usb_power, bool enable)
{
	struct axp_config_info *dts_info = &usb_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp_irq_set(axp2585_usb_irq[AXP2585_VIRQ_USBIN].irq,
				enable);
	if (!dts_info->wakeup_usb_out)
		axp_irq_set(axp2585_usb_irq[AXP2585_VIRQ_USBRE].irq,
				enable);
}

static int axp2585_usb_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp2585_usb_power *usb_power = platform_get_drvdata(pdev);

	axp2585_usb_virq_dts_set(usb_power, false);

	return 0;
}

static int axp2585_usb_power_resume(struct platform_device *pdev)
{
	struct axp2585_usb_power *usb_power = platform_get_drvdata(pdev);

	axp2585_usb_virq_dts_set(usb_power, true);

	return 0;
}

static const struct of_device_id axp2585_usb_power_match[] = {
	{
		.compatible = "x-powers,axp2585-usb-power-supply",
		.data = (void *)AXP2585_ID,
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp2585_usb_power_match);

static struct platform_driver axp2585_usb_power_driver = {
	.driver = {
		.name = "axp2585-usb-power-supply",
		.of_match_table = axp2585_usb_power_match,
	},
	.probe = axp2585_usb_power_probe,
	.remove = axp2585_usb_power_remove,
	.suspend = axp2585_usb_power_suspend,
	.resume = axp2585_usb_power_resume,
};

module_platform_driver(axp2585_usb_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp2585 usb power driver");
MODULE_LICENSE("GPL");
