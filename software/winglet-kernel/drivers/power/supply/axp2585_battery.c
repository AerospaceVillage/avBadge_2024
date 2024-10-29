#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include "linux/irq.h"
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/pm_runtime.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/err.h>
#include "linux/mfd/axp2101.h"
#include <linux/power/axp2585_battery.h>
#include "axp2101_charger.h"

#define AXP2585_VBAT_MAX        (8000)
#define AXP2585_VBAT_MIN        (2000)
#define AXP2585_SOC_MAX         (100)
#define AXP2585_SOC_MIN         (0)
#define AXP2585_MASK_VBUS_STATE BIT(1)
#define AXP2585_MODE_RSTGAUGE   BIT(3)
#define AXP2585_MODE_RSTMCU     BIT(2)
#define AXP2585_MAX_PARAM       512
#define AXP2585_BROMUP_EN       BIT(0)
#define AXP2585_CFG_UPDATE_MARK BIT(4)

#define AXP2585_CHARGING_TRI  (0)
#define AXP2585_CHARGING_PRE  (1)
#define AXP2585_CHARGING_CC   (2)
#define AXP2585_CHARGING_CV   (3)
#define AXP2585_CHARGING_DONE (4)
#define AXP2585_CHARGING_NCHG (5)
#define AXP2585_MANUFACTURER  "xpower,axp2585"

#define AXP2585_ADC_BATVOL_ENABLE   (1 << 4)
#define AXP2585_ADC_TSVOL_ENABLE    (1 << 5)
#define AXP2585_ADC_BATCUR_ENABLE   (1 << 6)
#define AXP2585_ADC_DIETMP_ENABLE   (1 << 7)

struct axp2585_device_info {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *bat;
	struct delayed_work        bat_chk;
	struct axp_config_info    dts_info;
	usbc_state_update_cb_t    usbc_update_cb;
	void*			  update_cb_arg;
	bool axp2585_version_d;
	struct notifier_block notifier;
};

RAW_NOTIFIER_HEAD(axp2585_notifier_list);
#ifdef CONFIG_AXP2585_LOW_WARNING_LED
#define LOW_WARNING_EVENT	0x5A
#endif

static int matches_driver(struct device_driver *driver);

static int axp2585_get_rest_vol(struct axp2585_device_info *di)
{
	u8 temp_val[2];
	u8 ocv_percent = 0;
	u8 coulomb_percent = 0;
	int batt_max_cap, coulumb_counter;
	int rest_vol;
	struct regmap *map = di->regmap;
	u32 val, tmp[2];
	bool bat_charging;

	regmap_read(map, AXP2585_CAP, &val);
	if (!(val & 0x80))
		return 0;
	rest_vol = (int) (val & 0x7F);
	regmap_read(map, 0xe4, &tmp[0]);
	if (tmp[0] & 0x80)
		ocv_percent = tmp[0] & 0x7f;

	regmap_read(map, 0xe5, &tmp[0]);
	if (tmp[0] & 0x80)
		coulomb_percent = tmp[0] & 0x7f;

	regmap_read(map, AXP2585_STATUS, &val);
	bat_charging = ((((val & (7 << 2)) > 0))
			&& ((val & (7 << 2)) < 0x14)) ? 1 : 0;
	if (ocv_percent == 100 && bat_charging == 0 && rest_vol == 99
		&& (val & BIT(1))) {
		regmap_update_bits(map, AXP2585_COULOMB_CTL, 0x00, 0x80);
		regmap_update_bits(map, AXP2585_COULOMB_CTL, 0x80, 0x80);
		rest_vol = 100;
	}
	regmap_bulk_read(map, 0xe2, temp_val, 2);
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;
	regmap_bulk_read(map, 0xe0, temp_val, 2);
	batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	return rest_vol;
}

static int axp2585_get_coulumb_counter(struct axp2585_device_info *di)
{
	u8 temp_val[2];
	struct regmap *map = di->regmap;
	int coulumb_counter;

	regmap_bulk_read(map, 0xe2, temp_val, 2);
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456;
	return coulumb_counter;
}

static int axp2585_get_bat(struct axp2585_device_info *di)
{
	u32 val;
	struct regmap *regmap = di->regmap;

	if (di->dts_info.pmu_bat_unused == 0) {
		if (di->axp2585_version_d) {
			regmap_read(regmap, 0x00, &val);
			if (!(val & 0x02))
				return 1;

			regmap_read(regmap, 0xbc, &val);
			if (val & 0xff)
				return 1;

			regmap_read(regmap, 0xbd, &val);
			if (val & 0xff)
				return 1;

			return 0;
		} else {
			regmap_read(regmap, 0x02, &val);
			if ((val & (3 << 3)) == 0x18)
				return 1;
			else
				return 0;
		}
	} else {
		return 0;
	}
}

static int axp2585_get_direction(struct axp2585_device_info *di)
{
	u32 val;
	struct regmap *regmap = di->regmap;
		regmap_read(regmap, 0x02, &val);
		if (val & 0x01)
			return 1;
		else
			return 0;
}

static int axp2585_get_bat_status(struct power_supply *psy,
				  union power_supply_propval *val)
{
	struct axp2585_device_info *di = power_supply_get_drvdata(psy);
	struct regmap *regmap = di->regmap;
	unsigned int data;
	int ret;
	bool bat_det, bat_charging;
	unsigned int rest_vol;
	bool usb_valid;

	ret = regmap_read(regmap, AXP2585_STATUS, &data);
	if (ret < 0) {
		dev_dbg(&psy->dev, "error read AXP210X_COM_STAT1\n");
		return ret;
	}

	bat_charging = ((((data & (7 << 2)) > 0))
			&& ((data & (7 << 2)) < 0x14)) ? 1 : 0;

	usb_valid = data & (1 << 1);

	bat_det = axp2585_get_bat(di);

	rest_vol = axp2585_get_rest_vol(di);

	if (bat_det) {
		if (rest_vol == 100)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else if (bat_charging)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
	}
	return 0;
}

static int axp2585_get_bat_health(void)
{
	return POWER_SUPPLY_HEALTH_GOOD;
}

static inline int axp2585_icharge_to_mA(u32 reg)
{
	return (int)((((reg >> 8) << 4) | (reg & 0x000f)) << 1);
}

static int axp2585_get_ibat(struct axp2585_device_info *di)
{
	struct regmap *map = di->regmap;
	u8 tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(map, AXP2585_IBATH_REG, tmp, 2);
	if (ret < 0)
		return ret;
	res = (tmp[0] << 8) | tmp[1];

	return axp2585_icharge_to_mA(res);
}

static inline int axp2585_vbat_to_mV(u32 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1200 / 1000;
}

static int axp2585_get_vbat(struct axp2585_device_info *di)
{
	struct regmap *map = di->regmap;
	u8 tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(map, AXP2585_VBATH_REG, tmp, 2);
	if (ret < 0)
		return ret;
	res = (tmp[0] << 8) | tmp[1];

	return axp2585_vbat_to_mV(res);
}

static int axp2585_get_vocv(struct axp2585_device_info *di)
{
	struct regmap *map = di->regmap;
	u8 tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(map, AXP2585_VOCVH_REG, tmp, 2);
	if (ret < 0)
		return ret;
	res = (tmp[0] << 8) | tmp[1];

	return axp2585_vbat_to_mV(res);
}

static inline int axp2585_ibat_to_mA(u32 reg)
{
	return (int)((((reg >> 8) << 4) | (reg & 0x000f)) << 1);
}

static int axp2585_get_disibat(struct axp2585_device_info *di)
{
	struct regmap *map = di->regmap;
	u8 tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(map, AXP2585_DISIBATH_REG, tmp, 2);
	if (ret < 0)
		return ret;
	res = (tmp[0] << 8) | tmp[1];

	return axp2585_ibat_to_mA(res);
}

static inline int axp_vts_to_mV(u16 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 800 / 1000;
}

static inline int axp_vts_to_temp(int data,
		const struct axp_config_info *axp_config)
{
	int temp;

	if (data < axp_config->pmu_bat_temp_para16)
		return 80;
	else if (data <= axp_config->pmu_bat_temp_para15) {
		temp = 70 + (axp_config->pmu_bat_temp_para15-data)*10/
		(axp_config->pmu_bat_temp_para15 - axp_config->pmu_bat_temp_para16);
	} else if (data <= axp_config->pmu_bat_temp_para14) {
		temp = 60 + (axp_config->pmu_bat_temp_para14-data)*10/
		(axp_config->pmu_bat_temp_para14 - axp_config->pmu_bat_temp_para15);
	} else if (data <= axp_config->pmu_bat_temp_para13) {
		temp = 55 + (axp_config->pmu_bat_temp_para13-data)*5/
		(axp_config->pmu_bat_temp_para13 - axp_config->pmu_bat_temp_para14);
	} else if (data <= axp_config->pmu_bat_temp_para12) {
		temp = 50 + (axp_config->pmu_bat_temp_para12-data)*5/
		(axp_config->pmu_bat_temp_para12 - axp_config->pmu_bat_temp_para13);
	} else if (data <= axp_config->pmu_bat_temp_para11) {
		temp = 45 + (axp_config->pmu_bat_temp_para11-data)*5/
		(axp_config->pmu_bat_temp_para11 - axp_config->pmu_bat_temp_para12);
	} else if (data <= axp_config->pmu_bat_temp_para10) {
		temp = 40 + (axp_config->pmu_bat_temp_para10-data)*5/
		(axp_config->pmu_bat_temp_para10 - axp_config->pmu_bat_temp_para11);
	} else if (data <= axp_config->pmu_bat_temp_para9) {
		temp = 30 + (axp_config->pmu_bat_temp_para9-data)*10/
		(axp_config->pmu_bat_temp_para9 - axp_config->pmu_bat_temp_para10);
	} else if (data <= axp_config->pmu_bat_temp_para8) {
		temp = 20 + (axp_config->pmu_bat_temp_para8-data)*10/
		(axp_config->pmu_bat_temp_para8 - axp_config->pmu_bat_temp_para9);
	} else if (data <= axp_config->pmu_bat_temp_para7) {
		temp = 10 + (axp_config->pmu_bat_temp_para7-data)*10/
		(axp_config->pmu_bat_temp_para7 - axp_config->pmu_bat_temp_para8);
	} else if (data <= axp_config->pmu_bat_temp_para6) {
		temp = 5 + (axp_config->pmu_bat_temp_para6-data)*5/
		(axp_config->pmu_bat_temp_para6 - axp_config->pmu_bat_temp_para7);
	} else if (data <= axp_config->pmu_bat_temp_para5) {
		temp = 0 + (axp_config->pmu_bat_temp_para5-data)*5/
		(axp_config->pmu_bat_temp_para5 - axp_config->pmu_bat_temp_para6);
	} else if (data <= axp_config->pmu_bat_temp_para4) {
		temp = -5 + (axp_config->pmu_bat_temp_para4-data)*5/
		(axp_config->pmu_bat_temp_para4 - axp_config->pmu_bat_temp_para5);
	} else if (data <= axp_config->pmu_bat_temp_para3) {
		temp = -10 + (axp_config->pmu_bat_temp_para3-data)*5/
		(axp_config->pmu_bat_temp_para3 - axp_config->pmu_bat_temp_para4);
	} else if (data <= axp_config->pmu_bat_temp_para2) {
		temp = -15 + (axp_config->pmu_bat_temp_para2-data)*5/
		(axp_config->pmu_bat_temp_para2 - axp_config->pmu_bat_temp_para3);
	} else if (data <= axp_config->pmu_bat_temp_para1) {
		temp = -25 + (axp_config->pmu_bat_temp_para1-data)*10/
		(axp_config->pmu_bat_temp_para1 - axp_config->pmu_bat_temp_para2);
	} else
		temp = -25;
	return temp;
}

static int axp2585_get_bat_temp(struct axp2585_device_info *di)
{
	unsigned char temp_val[2];
	unsigned short ts_res;
	int bat_temp_mv, bat_temp;
	int ret = 0;
	struct regmap *regmap = di->regmap;

	if (di->dts_info.pmu_bat_temp_enable == 0) {
		return INT_MIN;
	}

	ret = regmap_bulk_read(regmap, AXP803_VTS_RES, temp_val, 2);
	if (ret < 0)
		return ret;

	ts_res = ((unsigned short) temp_val[0] << 8) | temp_val[1];
	bat_temp_mv = axp_vts_to_mV(ts_res);
	bat_temp = axp_vts_to_temp(bat_temp_mv, &di->dts_info);

	pr_debug("bat_temp: %d\n", bat_temp);

	return bat_temp;
}

static enum power_supply_property axp2585_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static int axp2585_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;
	struct axp2585_device_info *di = power_supply_get_drvdata(psy);
	u8 temp_val[2];

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2585_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = axp2585_get_bat_health();
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = axp2585_get_coulumb_counter(di);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = di->dts_info.pmu_init_chgvol * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = di->dts_info.pmu_pwroff_vol * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		val->intval = axp2585_get_vocv(di) * 1000;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = axp2585_get_direction(di);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = axp2585_get_bat(di);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = axp2585_get_vbat(di) * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (axp2585_get_ibat(di) - axp2585_get_disibat(di)) * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = regmap_bulk_read(di->regmap, AXP2585_BATCAP0, temp_val, 2);
		if (ret < 0)
			return ret;
		val->intval = (((temp_val[0] & 0x7f) << 8) + temp_val[1]) * 1456;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = axp2585_get_rest_vol(di);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = axp2585_get_bat_temp(di);
		if (val->intval == INT_MIN) return -ENXIO;
		val->intval *= 10;  // Convert from C to tenths of degree C
		break;

	default:
		return -EINVAL;
	}
	return ret;
}

static int axp2585_register_battery(struct axp2585_device_info *di)
{
	int ret = 0;
	struct power_supply_desc *psy_desc;
	struct power_supply_config psy_cfg = {
		.of_node = di->dev->of_node,
		.drv_data = di,
	};

	psy_desc = devm_kzalloc(di->dev, sizeof(*psy_desc), GFP_KERNEL);
	if (!psy_desc)
		return -ENOMEM;

	psy_desc->name = "battery";
	psy_desc->type = POWER_SUPPLY_TYPE_BATTERY;
	psy_desc->properties = axp2585_props;
	psy_desc->num_properties = ARRAY_SIZE(axp2585_props);
	psy_desc->get_property = axp2585_get_property;
	psy_desc->use_for_apm = 1;

	di->bat = power_supply_register(di->dev, psy_desc, &psy_cfg);
	if (IS_ERR(di->bat)) {
		dev_err(di->dev, "failed to register battery\n");
		ret = PTR_ERR(di->bat);
		goto err1;
	}

	return ret;
err1:
	power_supply_unregister(di->bat);

	return ret;
}

static void axp2585_teardown_battery(struct axp2585_device_info *di)
{
	if (di->bat)
		power_supply_unregister(di->bat);

}

/* show pwm-led when battery capacity low. */
#ifdef CONFIG_AXP2585_LOW_WARNING_LED
irqreturn_t axp2585_irq_handler_low_warn1(int irq, void *data)
{
	struct axp2585_device_info *di = data;

	pr_debug("%s: enter interrupt %d\n", __func__, irq);
	pr_debug("==========battery capacity low warning!!==========\n");
	pr_debug("=============need to charge!!=============\n");

	power_supply_changed(di->bat);

	/* call notifier to show pwm-led*/
	raw_notifier_call_chain(&axp2585_notifier_list, LOW_WARNING_EVENT, NULL);

	return IRQ_HANDLED;
}
#endif

static irqreturn_t axp2585_irq_warning2(int irq, void* data)
{
	struct axp2585_device_info *di = data;
	power_supply_changed(di->bat);

	return IRQ_HANDLED;
}

static irqreturn_t axp2585_irq_handler_thread(int irq, void *data)
{
	struct axp2585_device_info *di = data;

	pr_debug("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(di->bat);
	return IRQ_HANDLED;
}

static irqreturn_t axp2585_usbc_state_update_irq(int irq, void *data)
{
	struct axp2585_device_info *di = data;

	pr_debug("%s: USBC State Update: %d\n", __func__, irq);

	if (di->usbc_update_cb) {
		di->usbc_update_cb(di->update_cb_arg);
	}

	power_supply_changed(di->bat);
	return IRQ_HANDLED;
}

enum axp2585_virq_index {
	AXP2585_VIRQ_BAT_IN,
	AXP2585_VIRQ_BAT_OUT,
	AXP2585_VIRQ_CHARGING,
	AXP2585_VIRQ_CHARGE_OVER,
	AXP2585_VIRQ_LOW_WARNING1,
	AXP2585_VIRQ_LOW_WARNING2,
	AXP2585_VIRQ_TC_IN,
	AXP2585_VIRQ_TC_OUT,
	AXP2585_VIRQ_BAT_UNTEMP_WORK,
	AXP2585_VIRQ_BAT_OVTEMP_WORK,
	AXP2585_VIRQ_BAT_UNTEMP_CHG,
	AXP2585_VIRQ_BAT_OVTEMP_CHG,

	AXP2585_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_charger_irq[] = {
	[AXP2585_VIRQ_BAT_IN] = { "bat in", axp2585_irq_handler_thread },
	[AXP2585_VIRQ_BAT_OUT] = { "bat out", axp2585_irq_handler_thread },
	[AXP2585_VIRQ_CHARGING] = { "charging", axp2585_irq_handler_thread },
	[AXP2585_VIRQ_CHARGE_OVER] = { "charge over",
						axp2585_irq_handler_thread },
#ifdef CONFIG_AXP2585_LOW_WARNING_LED
	[AXP2585_VIRQ_LOW_WARNING1] = { "low warning1",
						axp2585_irq_handler_low_warn1 },
#else
	[AXP2585_VIRQ_LOW_WARNING1] = { "low warning1",
						axp2585_irq_handler_thread },
#endif
	[AXP2585_VIRQ_LOW_WARNING2] = { "low warning2",
						axp2585_irq_warning2 },
	[AXP2585_VIRQ_TC_IN] = { "tc in", axp2585_usbc_state_update_irq },
	[AXP2585_VIRQ_TC_OUT] = { "tc out", axp2585_usbc_state_update_irq },
	[AXP2585_VIRQ_BAT_UNTEMP_WORK] = { "bat untemp work",
						axp2585_irq_handler_thread },
	[AXP2585_VIRQ_BAT_OVTEMP_WORK] = { "bat ovtemp work",
						axp2585_irq_handler_thread },
	[AXP2585_VIRQ_BAT_UNTEMP_CHG] = { "bat untemp chg",
						axp2585_irq_handler_thread },
	[AXP2585_VIRQ_BAT_OVTEMP_CHG] = { "bat ovtemp chg",
						axp2585_irq_handler_thread },
};

static int axp2585_notifier_event(struct notifier_block *nb, unsigned long event, void *v)
{
	switch (event) {
#ifdef CONFIG_AXP2585_LOW_WARNING_LED
int axp_leds_show(void);
	case LOW_WARNING_EVENT:
		axp_leds_show();
		break;
#endif

	default:
		pr_err("axp2585: unknow notifier event\n");
		break;
	}

	return NOTIFY_DONE;
}

static int axp2585_notifier_init(struct axp2585_device_info *di)
{
	di->notifier.notifier_call = axp2585_notifier_event;
	raw_notifier_chain_register(&axp2585_notifier_list, &di->notifier);

	return 0;
}

static void  axp2585_set_chg_cur(struct regmap *regmap, u32 cur)
{
	u8 tmp = 0;

	tmp = cur / 64;
	if (tmp > 0x3f)
		tmp = 0x3f;

	regmap_update_bits(regmap, 0x8b, 0x3f, tmp);
}

struct platform_device *axp2585_batt_by_phandle(struct device *dev, int index)
{
	struct device_node *node, *np = dev_of_node(dev);

	if (!np) {
		dev_dbg(dev, "device does not have a device node entry\n");
		return ERR_PTR(-EINVAL);
	}

	node = of_parse_phandle(np, "axp2585-bmu", index);
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
EXPORT_SYMBOL(axp2585_batt_by_phandle);

void axp2585_batt_register_det_cb(struct platform_device *pdev,
					usbc_state_update_cb_t cb, void* arg)
{

	struct axp2585_device_info *di = platform_get_drvdata(pdev);
	WARN(di->usbc_update_cb != NULL && di->usbc_update_cb != cb,
		"USBC Detect Callback already registered");

	di->usbc_update_cb = cb;
	di->update_cb_arg = arg;
}
EXPORT_SYMBOL(axp2585_batt_register_det_cb);

int axp2585_batt_get_id_mode(struct platform_device *pdev,
				enum axp2585_usb_id_mode *mode_out) {

	struct axp2585_device_info *di = platform_get_drvdata(pdev);
	struct regmap *regmap = di->regmap;
	u32 val;
	int ret;

	ret = regmap_read(regmap, AXP2585_CC_STATUS0, &val);
	if (ret) {
		return ret;
	}
	val &= 0xF;  // Only grab CC logic state

	switch (val) {
		case 0b0110:	// ATTACH_SRC
			*mode_out = AXP2585_ID_HOST;
			break;
		default:
			// All other modes fall back to peripheral mode
			*mode_out = AXP2585_ID_PERIPHERAL;
			break;
	}

	return 0;
}
EXPORT_SYMBOL(axp2585_batt_get_id_mode);

static void axp2585_charger_init(struct axp2585_device_info *di)
{
	unsigned char ocv_cap[32];
	unsigned int val;
	int cur_coulomb_counter, rdc;
	int i;
	int update_min_times[8] = {30, 60, 120, 164, 0, 5, 10, 20};
	int ocv_cou_adjust_time[4] = {60, 120, 15, 30};

	struct regmap *regmap = di->regmap;
	struct axp_config_info *dinfo = &di->dts_info;

	/* set chg time */
	if (dinfo->pmu_init_chg_pretime < 40)
		dinfo->pmu_init_chg_pretime = 40;
	val = (dinfo->pmu_init_chg_pretime - 40)/10;
	if (val >= 3)
		val = 3;
	val = 0x80 + (val << 5);
	regmap_update_bits(regmap, 0x8e, 0xe0, val);

	if (dinfo->pmu_init_chg_csttime <= 60 * 5)
		val = 0;
	else if (dinfo->pmu_init_chg_csttime <= 60 * 8)
		val = 1;
	else if (dinfo->pmu_init_chg_csttime <= 60 * 12)
		val = 2;
	else if (dinfo->pmu_init_chg_csttime <= 60 * 20)
		val = 3;
	else
		val = 3;
	val = (val << 1) + 0x01;
	regmap_update_bits(regmap, 0x8d, 0x07, val);

	/* adc set */
	val = AXP2585_ADC_BATVOL_ENABLE | AXP2585_ADC_BATCUR_ENABLE;
	if (dinfo->pmu_bat_temp_enable != 0)
		val = val | AXP2585_ADC_TSVOL_ENABLE;
	regmap_update_bits(regmap, AXP2585_ADC_CONTROL,
					AXP2585_ADC_BATVOL_ENABLE
					| AXP2585_ADC_BATCUR_ENABLE
					| AXP2585_ADC_TSVOL_ENABLE,
					val);

	regmap_read(regmap, AXP2585_TS_PIN_CONTROL, &val);
	switch (dinfo->pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(3 << 5);
		break;
	case 2:
		val &= ~(3 << 5);
		val |= 1 << 5;
		break;
	case 4:
		val &= ~(3 << 5);
		val |= 2 << 5;
		break;
	case 8:
		val &= ~(3 << 5);
		val |= 3 << 5;
		break;
	default:
		break;
	}

	if (dinfo->pmu_bat_temp_enable != 0)
		val &= ~(1 << 7);
	regmap_write(regmap, AXP2585_TS_PIN_CONTROL, val);

	/* bat para */
	regmap_write(regmap, AXP2585_WARNING_LEVEL,
		((dinfo->pmu_battery_warning_level1 - 5) << 4)
		+ dinfo->pmu_battery_warning_level2);

	if (dinfo->pmu_init_chgvol < 3840)
		dinfo->pmu_init_chgvol = 3840;
	val = (dinfo->pmu_init_chgvol - 3840)/16;
	if (val > 0x30)
		val = 0x30;
	val <<= 2;
	regmap_update_bits(regmap, AXP2585_CHARGE_CONTROL2, 0xfc, val);

	ocv_cap[0]  = dinfo->pmu_bat_para1;
	ocv_cap[1]  = dinfo->pmu_bat_para2;
	ocv_cap[2]  = dinfo->pmu_bat_para3;
	ocv_cap[3]  = dinfo->pmu_bat_para4;
	ocv_cap[4]  = dinfo->pmu_bat_para5;
	ocv_cap[5]  = dinfo->pmu_bat_para6;
	ocv_cap[6]  = dinfo->pmu_bat_para7;
	ocv_cap[7]  = dinfo->pmu_bat_para8;
	ocv_cap[8]  = dinfo->pmu_bat_para9;
	ocv_cap[9]  = dinfo->pmu_bat_para10;
	ocv_cap[10] = dinfo->pmu_bat_para11;
	ocv_cap[11] = dinfo->pmu_bat_para12;
	ocv_cap[12] = dinfo->pmu_bat_para13;
	ocv_cap[13] = dinfo->pmu_bat_para14;
	ocv_cap[14] = dinfo->pmu_bat_para15;
	ocv_cap[15] = dinfo->pmu_bat_para16;
	ocv_cap[16] = dinfo->pmu_bat_para17;
	ocv_cap[17] = dinfo->pmu_bat_para18;
	ocv_cap[18] = dinfo->pmu_bat_para19;
	ocv_cap[19] = dinfo->pmu_bat_para20;
	ocv_cap[20] = dinfo->pmu_bat_para21;
	ocv_cap[21] = dinfo->pmu_bat_para22;
	ocv_cap[22] = dinfo->pmu_bat_para23;
	ocv_cap[23] = dinfo->pmu_bat_para24;
	ocv_cap[24] = dinfo->pmu_bat_para25;
	ocv_cap[25] = dinfo->pmu_bat_para26;
	ocv_cap[26] = dinfo->pmu_bat_para27;
	ocv_cap[27] = dinfo->pmu_bat_para28;
	ocv_cap[28] = dinfo->pmu_bat_para29;
	ocv_cap[29] = dinfo->pmu_bat_para30;
	ocv_cap[30] = dinfo->pmu_bat_para31;
	ocv_cap[31] = dinfo->pmu_bat_para32;
	regmap_bulk_write(regmap, 0xC0, ocv_cap, 32);

	/* Init CHGLED function */
	if (dinfo->pmu_chgled_func)
		regmap_update_bits(regmap, 0x90, 0x80, 0x80);
	else
		regmap_update_bits(regmap, 0x90, 0x80, 0x00);

	regmap_update_bits(regmap, 0x90, 0x07, dinfo->pmu_chgled_type);

	/* init battery capacity correct function */
	if (dinfo->pmu_batt_cap_correct)
		regmap_update_bits(regmap, 0xb8, 0x20, 0x20);
	else
		regmap_update_bits(regmap, 0xb8, 0x20, 0x0);

	/* battery detect enable */
	if (!(di->axp2585_version_d)) {
		if (dinfo->pmu_batdeten)
			regmap_update_bits(regmap, 0x8e, 0x08, 0x08);
		else
			regmap_update_bits(regmap, 0x8e, 0x08, 0x0);
	}

	/* RDC initial */
	regmap_read(regmap, AXP2585_RDC0, &val);
	if ((dinfo->pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (dinfo->pmu_battery_rdc * 10000 + 5371) / 10742;
		regmap_write(regmap, AXP2585_RDC0, ((rdc >> 8)& 0x1F)|0x80);
		regmap_write(regmap, AXP2585_RDC1, rdc & 0x00FF);
	}

	regmap_read(regmap, AXP2585_BATCAP0, &val);
	if ((dinfo->pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = dinfo->pmu_battery_cap
					* 1000 / 1456;
		regmap_write(regmap, AXP2585_BATCAP0,
					((cur_coulomb_counter >> 8) | 0x80));
		regmap_write(regmap, AXP2585_BATCAP1,
					cur_coulomb_counter & 0x00FF);
	} else if (!dinfo->pmu_battery_cap) {
		regmap_write(regmap, AXP2585_BATCAP0, 0x00);
		regmap_write(regmap, AXP2585_BATCAP1, 0x00);
	}

	if (dinfo->pmu_bat_temp_enable != 0) {
		regmap_write(regmap, AXP2585_VLTF_CHARGE,
				dinfo->pmu_bat_charge_ltf * 10 / 128);
		regmap_write(regmap, AXP2585_VHTF_CHARGE,
				dinfo->pmu_bat_charge_htf * 10 / 128);
		regmap_write(regmap, AXP2585_VLTF_WORK,
				dinfo->pmu_bat_shutdown_ltf * 10 / 128);
		regmap_write(regmap, AXP2585_VHTF_WORK,
				dinfo->pmu_bat_shutdown_htf * 10 / 128);
	}
	/*enable fast charge */
	// regmap_update_bits(regmap, 0x31, 0x04, 0x04);
	/*set POR time as 12s*/
	regmap_update_bits(regmap, AXP2585_POK_SET, 0x30, 0x20);
	/* shut off timer in dpm */
	regmap_update_bits(regmap, AXP2585_TIMER2_SET, 0x10, 0x10);
	for (i = 0; i < ARRAY_SIZE(update_min_times); i++) {
		if (update_min_times[i] == dinfo->pmu_update_min_time)
			break;
	}
	regmap_update_bits(regmap, AXP2585_ADJUST_PARA, 0x7, i);
	/*initial the ocv_cou_adjust_time*/
	for (i = 0; i < ARRAY_SIZE(ocv_cou_adjust_time); i++) {
		if (ocv_cou_adjust_time[i] == dinfo->pmu_ocv_cou_adjust_time)
			break;
	}
	i <<= 6;
	regmap_update_bits(regmap, AXP2585_ADJUST_PARA1, 0xc0, i);

	/* init the chg runtime_cur */
	axp2585_set_chg_cur(regmap, dinfo->pmu_runtime_chgcur);

	/*type-c cc logic init*/
#ifdef CONFIG_TYPE_C
	regmap_update_bits(regmap, AXP2585_CC_EN, 0x02, 0x02);
	regmap_update_bits(regmap, AXP2585_CC_LOW_POWER_CTRL, 0x04, 0x00);
	regmap_update_bits(regmap, AXP2585_CC_MODE_CTRL, 0x03, 0x03);
#endif
}

#define AXP_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &axp_config->name))\
		axp_config->name = def_value;\
} while (0)

int axp2585_charger_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}
	AXP_OF_PROP_READ(pmu_battery_rdc,              BATRDC);
	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_batdeten,                      1);
	AXP_OF_PROP_READ(pmu_chg_ic_temp,                   0);
	AXP_OF_PROP_READ(pmu_runtime_chgcur, INTCHGCUR / 1000);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_init_chgvol,    INTCHGVOL / 1000);
	AXP_OF_PROP_READ(pmu_init_chgend_rate,  INTCHGENDRATE);
	AXP_OF_PROP_READ(pmu_init_chg_enabled,              1);
	AXP_OF_PROP_READ(pmu_init_bc_en,                    0);
	AXP_OF_PROP_READ(pmu_init_adc_freq,        INTADCFREQ);
	AXP_OF_PROP_READ(pmu_init_adcts_freq,     INTADCFREQC);
	AXP_OF_PROP_READ(pmu_init_chg_pretime,  INTCHGPRETIME);
	AXP_OF_PROP_READ(pmu_init_chg_csttime,  INTCHGCSTTIME);
	AXP_OF_PROP_READ(pmu_batt_cap_correct,              1);
	AXP_OF_PROP_READ(pmu_chg_end_on_en,                 0);
	AXP_OF_PROP_READ(ocv_coulumb_100,                   0);
	AXP_OF_PROP_READ(pmu_bat_para1,               OCVREG0);
	AXP_OF_PROP_READ(pmu_bat_para2,               OCVREG1);
	AXP_OF_PROP_READ(pmu_bat_para3,               OCVREG2);
	AXP_OF_PROP_READ(pmu_bat_para4,               OCVREG3);
	AXP_OF_PROP_READ(pmu_bat_para5,               OCVREG4);
	AXP_OF_PROP_READ(pmu_bat_para6,               OCVREG5);
	AXP_OF_PROP_READ(pmu_bat_para7,               OCVREG6);
	AXP_OF_PROP_READ(pmu_bat_para8,               OCVREG7);
	AXP_OF_PROP_READ(pmu_bat_para9,               OCVREG8);
	AXP_OF_PROP_READ(pmu_bat_para10,              OCVREG9);
	AXP_OF_PROP_READ(pmu_bat_para11,              OCVREGA);
	AXP_OF_PROP_READ(pmu_bat_para12,              OCVREGB);
	AXP_OF_PROP_READ(pmu_bat_para13,              OCVREGC);
	AXP_OF_PROP_READ(pmu_bat_para14,              OCVREGD);
	AXP_OF_PROP_READ(pmu_bat_para15,              OCVREGE);
	AXP_OF_PROP_READ(pmu_bat_para16,              OCVREGF);
	AXP_OF_PROP_READ(pmu_bat_para17,             OCVREG10);
	AXP_OF_PROP_READ(pmu_bat_para18,             OCVREG11);
	AXP_OF_PROP_READ(pmu_bat_para19,             OCVREG12);
	AXP_OF_PROP_READ(pmu_bat_para20,             OCVREG13);
	AXP_OF_PROP_READ(pmu_bat_para21,             OCVREG14);
	AXP_OF_PROP_READ(pmu_bat_para22,             OCVREG15);
	AXP_OF_PROP_READ(pmu_bat_para23,             OCVREG16);
	AXP_OF_PROP_READ(pmu_bat_para24,             OCVREG17);
	AXP_OF_PROP_READ(pmu_bat_para25,             OCVREG18);
	AXP_OF_PROP_READ(pmu_bat_para26,             OCVREG19);
	AXP_OF_PROP_READ(pmu_bat_para27,             OCVREG1A);
	AXP_OF_PROP_READ(pmu_bat_para28,             OCVREG1B);
	AXP_OF_PROP_READ(pmu_bat_para29,             OCVREG1C);
	AXP_OF_PROP_READ(pmu_bat_para30,             OCVREG1D);
	AXP_OF_PROP_READ(pmu_bat_para31,             OCVREG1E);
	AXP_OF_PROP_READ(pmu_bat_para32,             OCVREG1F);
	AXP_OF_PROP_READ(pmu_ac_vol,                     4400);
	AXP_OF_PROP_READ(pmu_ac_cur,                        0);
	AXP_OF_PROP_READ(pmu_pwroff_vol,                 3300);
	AXP_OF_PROP_READ(pmu_pwron_vol,                  2900);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_restvol_adjust_time,          30);
	AXP_OF_PROP_READ(pmu_ocv_cou_adjust_time,          60);
	AXP_OF_PROP_READ(pmu_chgled_func,                   0);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);
	AXP_OF_PROP_READ(pmu_bat_temp_enable,               0);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             0xA5);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,             0x1F);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           0xFC);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,           0x16);
	AXP_OF_PROP_READ(pmu_bat_temp_para1,             7466);
	AXP_OF_PROP_READ(pmu_bat_temp_para2,             4480);
	AXP_OF_PROP_READ(pmu_bat_temp_para3,             3518);
	AXP_OF_PROP_READ(pmu_bat_temp_para4,             2786);
	AXP_OF_PROP_READ(pmu_bat_temp_para5,             2223);
	AXP_OF_PROP_READ(pmu_bat_temp_para6,             1788);
	AXP_OF_PROP_READ(pmu_bat_temp_para7,             1448);
	AXP_OF_PROP_READ(pmu_bat_temp_para8,              969);
	AXP_OF_PROP_READ(pmu_bat_temp_para9,              664);
	AXP_OF_PROP_READ(pmu_bat_temp_para10,             466);
	AXP_OF_PROP_READ(pmu_bat_temp_para11,             393);
	AXP_OF_PROP_READ(pmu_bat_temp_para12,             333);
	AXP_OF_PROP_READ(pmu_bat_temp_para13,             283);
	AXP_OF_PROP_READ(pmu_bat_temp_para14,             242);
	AXP_OF_PROP_READ(pmu_bat_temp_para15,             179);
	AXP_OF_PROP_READ(pmu_bat_temp_para16,             134);
	AXP_OF_PROP_READ(pmu_bat_unused,                    0);
	AXP_OF_PROP_READ(power_start,                       0);
	AXP_OF_PROP_READ(pmu_ocv_en,                        1);
	AXP_OF_PROP_READ(pmu_cou_en,                        1);
	AXP_OF_PROP_READ(pmu_update_min_time,   UPDATEMINTIME);
	AXP_OF_PROP_READ(batt_shutoff_voltage,           3000);
	AXP_OF_PROP_READ(batt_min_detect_voltage,        1500);

	axp_config->wakeup_bat_in =
		of_property_read_bool(node, "wakeup_bat_in");
	axp_config->wakeup_bat_out =
		of_property_read_bool(node, "wakeup_bat_out");
	axp_config->wakeup_bat_charging =
		of_property_read_bool(node, "wakeup_bat_charging");
	axp_config->wakeup_bat_charge_over =
		of_property_read_bool(node, "wakeup_bat_charge_over");
	axp_config->wakeup_low_warning1 =
		of_property_read_bool(node, "wakeup_low_warning1");
	axp_config->wakeup_low_warning2 =
		of_property_read_bool(node, "wakeup_low_warning2");
	axp_config->wakeup_bat_untemp_work =
		of_property_read_bool(node, "wakeup_bat_untemp_work");
	axp_config->wakeup_bat_ovtemp_work =
		of_property_read_bool(node, "wakeup_bat_ovtemp_work");
	axp_config->wakeup_untemp_chg =
		of_property_read_bool(node, "wakeup_bat_untemp_chg");
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");
//	axp_config->wakeup_tc_in =
//		of_property_read_bool(node, "wakeup_tc_in");
//	axp_config->wakeup_tc_out =
//		of_property_read_bool(node, "wakeup_tc_out");

	return 0;
}

static void axp2585_parse_device_tree(struct axp2585_device_info *di)
{
	int ret;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!di->dev->of_node) {
		pr_info("can not find device tree\n");
		return;
	}

	cfg = &di->dts_info;
	ret = axp2585_charger_dt_parse(di->dev->of_node, cfg);
	if (ret) {
		pr_info("can not parse device tree err\n");
		return;
	}

}

static void axp2585_bat_power_monitor(struct work_struct *work)
{
	struct axp2585_device_info *di =
		container_of(work, typeof(*di), bat_chk.work);

	power_supply_changed(di->bat);

	int bat_voltage = axp2585_get_vbat(di);
	if (bat_voltage > di->dts_info.batt_min_detect_voltage &&
			bat_voltage <= di->dts_info.batt_shutoff_voltage) {
		pr_emerg("%s: Battery Critically Low! Shutting Down!\n", __func__);
		orderly_poweroff(true);
	}

	schedule_delayed_work(&di->bat_chk, msecs_to_jiffies(10*1000));
}

static int axp2585_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;
	unsigned int val;
	struct axp2585_device_info *di;
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (!axp_dev->irq) {
		pr_err("can not register axp2585-charger without irq\n");
		return -EINVAL;
	}

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (di == NULL) {
		pr_err("axp2585_device_info alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	di->name = "axp2585_power";
	di->dev = &pdev->dev;
	di->regmap = axp_dev->regmap;
	di->usbc_update_cb = NULL;

	/* for device tree parse */
	axp2585_parse_device_tree(di);

	/* check bmu1760 d or c version */
	regmap_read(di->regmap, 0x03, &val);
	di->axp2585_version_d = (val & 0x20);

	axp2585_charger_init(di);

	ret = axp2585_register_battery(di);
	if (ret < 0) {
		dev_err(di->dev, "axp210x register battery dev fail!\n");
		goto err;
	}

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			dev_err(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_charger_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_charger_irq[i].isr, 0,
						   axp_charger_irq[i].name, di);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_charger_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_charger_irq[i].name, irq, ret);
	}

	platform_set_drvdata(pdev, di);
	INIT_DELAYED_WORK(&di->bat_chk, axp2585_bat_power_monitor);
	schedule_delayed_work(&di->bat_chk, msecs_to_jiffies(10 * 1000));

	axp2585_notifier_init(di);

	return ret;

err:
	pr_err("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp2585_charger_remove(struct platform_device *pdev)
{
	struct axp2585_device_info *di = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "==============AXP2585 unegister==============\n");
	axp2585_teardown_battery(di);
	dev_dbg(&pdev->dev, "axp210x teardown battery dev\n");

	return 0;
}

static void axp2585_icchg_set(struct axp2585_device_info *di, int mA)
{

	mA = mA / 64;
	if (mA > 0x3f)
		mA = 0x3f;
	/* bit 5:0 is the ctrl bit */
	regmap_update_bits(di->regmap, AXP2585_ICC_CFG, GENMASK(5, 0), mA);
}

static inline void axp2585_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2585_virq_dts_set(struct axp2585_device_info *di, bool enable)
{
	struct axp_config_info *dts_info = &di->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_BAT_IN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_BAT_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_CHARGING].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_CHARGE_OVER].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_LOW_WARNING1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_LOW_WARNING2].irq,
				enable);
//	if (!dts_info->wakeup_tc_in)
//		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_TC_IN].irq,
//				enable);
//	if (!dts_info->wakeup_tc_out)
//		axp2585_irq_set(axp_charger_irq[AXP2585_VIRQ_TC_OUT].irq,
//				enable);

	if (!dts_info->wakeup_bat_untemp_work)
		axp2585_irq_set(
			axp_charger_irq[AXP2585_VIRQ_BAT_UNTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp2585_irq_set(
			axp_charger_irq[AXP2585_VIRQ_BAT_OVTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_untemp_chg)
		axp2585_irq_set(
			axp_charger_irq[AXP2585_VIRQ_BAT_UNTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp2585_irq_set(
			axp_charger_irq[AXP2585_VIRQ_BAT_OVTEMP_CHG].irq,
			enable);

}

static void axp2585_shutdown(struct platform_device *p)
{
	struct axp2585_device_info *di = platform_get_drvdata(p);

	axp2585_icchg_set(di, di->dts_info.pmu_shutdown_chgcur);

}

static int axp2585_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp2585_device_info *di = platform_get_drvdata(p);

	axp2585_icchg_set(di, di->dts_info.pmu_suspend_chgcur);

	axp2585_virq_dts_set(di, false);
	return 0;
}

static int axp2585_resume(struct platform_device *p)
{
	struct axp2585_device_info *di = platform_get_drvdata(p);

	axp2585_icchg_set(di, di->dts_info.pmu_runtime_chgcur);

	axp2585_virq_dts_set(di, true);

	return 0;
}

static const struct of_device_id axp2585_power_of_match[] = {
	{ .compatible = "x-powers,axp2585-battery-power-supply", },
	{}
};
MODULE_DEVICE_TABLE(of, axp2585_power_of_match);

static struct platform_driver axp2585_charger_driver = {
	.driver = {
		.name = "axp2585-battery-power-supply",
		.of_match_table = axp2585_power_of_match,
	},
	.probe = axp2585_battery_probe,
	.remove = axp2585_charger_remove,
	.shutdown = axp2585_shutdown,
	.suspend = axp2585_suspend,
	.resume = axp2585_resume,
};

module_platform_driver(axp2585_charger_driver);

static int matches_driver(struct device_driver *driver)
{
	return driver == &axp2585_charger_driver.driver;
}

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp2585 i2c driver");
MODULE_LICENSE("GPL");
