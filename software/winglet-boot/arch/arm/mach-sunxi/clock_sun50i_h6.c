#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/prcm.h>
#include <linux/delay.h>

#ifdef CONFIG_SPL_BUILD
void clock_init_safe(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	struct sunxi_prcm_reg *const prcm =
		(struct sunxi_prcm_reg *)SUNXI_PRCM_BASE;

	if (IS_ENABLED(CONFIG_MACH_SUN50I_H616)) {
		/* this seems to enable PLLs on H616 */
		setbits_le32(&prcm->sys_pwroff_gating, 0x10);
		setbits_le32(&prcm->res_cal_ctrl, 2);
	}

	if (IS_ENABLED(CONFIG_MACH_SUN50I_H616) ||
	    IS_ENABLED(CONFIG_MACH_SUN50I_H6)) {
		clrbits_le32(&prcm->res_cal_ctrl, 1);
		setbits_le32(&prcm->res_cal_ctrl, 1);
	}

	if (IS_ENABLED(CONFIG_MACH_SUN50I_H6)) {
		/* set key field for ldo enable */
		setbits_le32(&prcm->pll_ldo_cfg, 0xA7000000);
		/* set PLL VDD LDO output to 1.14 V */
		setbits_le32(&prcm->pll_ldo_cfg, 0x60000);
	}

	clock_set_pll1(408000000);

	writel(CCM_PLL6_DEFAULT, &ccm->pll6_cfg);
	while (!(readl(&ccm->pll6_cfg) & CCM_PLL6_LOCK))
		;

	clrsetbits_le32(&ccm->cpu_axi_cfg, CCM_CPU_AXI_APB_MASK | CCM_CPU_AXI_AXI_MASK,
			CCM_CPU_AXI_DEFAULT_FACTORS);

	writel(CCM_PSI_AHB1_AHB2_DEFAULT, &ccm->psi_ahb1_ahb2_cfg);
#ifdef CCM_AHB3_DEFAULT
	writel(CCM_AHB3_DEFAULT, &ccm->ahb3_cfg);
#endif
	writel(CCM_APB1_DEFAULT, &ccm->apb1_cfg);

	/*
	 * The mux and factor are set, but the clock will be enabled in
	 * DRAM initialization code.
	 */
	writel(MBUS_CLK_SRC_PLL6X2 | MBUS_CLK_M(3), &ccm->mbus_cfg);
}
#endif

void clock_init_uart(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* uart clock source is apb2 */
	writel(APB2_CLK_SRC_OSC24M|
	       APB2_CLK_RATE_N_1|
	       APB2_CLK_RATE_M(1),
	       &ccm->apb2_cfg);

	/* open the clock for uart */
	setbits_le32(&ccm->uart_gate_reset,
		     1 << (CONFIG_CONS_INDEX - 1));

	/* deassert uart reset */
	setbits_le32(&ccm->uart_gate_reset,
		     1 << (RESET_SHIFT + CONFIG_CONS_INDEX - 1));
}

#ifdef CONFIG_SPL_BUILD
void clock_set_pll1(unsigned int clk)
{
	struct sunxi_ccm_reg * const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	u32 val;

	/* Do not support clocks < 288MHz as they need factor P */
	if (clk < 288000000) clk = 288000000;

	/* Switch to 24MHz clock while changing PLL1 */
	val = readl(&ccm->cpu_axi_cfg);
	val &= ~CCM_CPU_AXI_MUX_MASK;
	val |= CCM_CPU_AXI_MUX_OSC24M;
	writel(val, &ccm->cpu_axi_cfg);

	/* clk = 24*n/p, p is ignored if clock is >288MHz */
	val = CCM_PLL1_CTRL_EN | CCM_PLL1_LOCK_EN | CCM_PLL1_CLOCK_TIME_2;
	val |= CCM_PLL1_CTRL_N(clk / 24000000);
	if (IS_ENABLED(CONFIG_MACH_SUN50I_H616))
	       val |= CCM_PLL1_OUT_EN;
	if (IS_ENABLED(CONFIG_SUNXI_GEN_NCAT2))
	       val |= CCM_PLL1_OUT_EN | CCM_PLL1_LDO_EN;
	writel(val, &ccm->pll1_cfg);
	while (!(readl(&ccm->pll1_cfg) & CCM_PLL1_LOCK)) {}

	/* Switch CPU to PLL1 */
	val = readl(&ccm->cpu_axi_cfg);
	val &= ~CCM_CPU_AXI_MUX_MASK;
	val |= CCM_CPU_AXI_MUX_PLL_CPUX;
	writel(val, &ccm->cpu_axi_cfg);
}
#endif

void clock_set_pll3(unsigned int clk)
{
	if (clk == 0) {
		clock_set_pll3_factors(0, 0);
		return;
	}

	// Right now, always have m = 2, we should have enough flexibility with this, we should hopefully never need > 6GHz
	const int m = 2;

	/* PLL3 rate = 24000000 * n / m */
	int mul = clk / (24000000 / m);

	clock_set_pll3_factors(mul, m > 1);
}

void clock_set_pll3_factors(int mul, int div2) {
	struct sunxi_ccm_reg * const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	if (mul == 0) {
		clrbits_le32(&ccm->pll3_cfg, CCM_PLL3_CTRL_EN);
		return;
	}

	unsigned int old_val = readl(&ccm->pll3_cfg);
	int old_mul = (old_val & CCM_PLL3_CTRL_N_MASK) >> CCM_PLL3_CTRL_N_SHIFT;
	int old_div2 = (old_val & CCM_PLL3_CTRL_DIV2_MASK) >> CCM_PLL3_CTRL_DIV2_SHIFT;
	bool was_enabled = !!(old_val & CCM_PLL3_CTRL_EN);
	bool was_locked = !!(old_val & CCM_PLL3_LOCK);
	bool was_lock_en = !!(old_val & CCM_PLL3_LOCK_EN);

	if (old_mul == mul && old_div2 == div2 && was_enabled && was_locked && was_lock_en) {
		// PLL already configured how we'd like, nothing to do
		return;
	}

	if (was_enabled) {
		/* Already enabled, follow steps in User Manual Section 3.3.4.3 */
		/* Disable PLL Output Enable During Configure */
		clrbits_le32(&ccm->pll3_cfg, CCM_PLL3_OUT_EN);
	}

	/* Configure N & M Factors */
	clrsetbits_le32(&ccm->pll3_cfg, CCM_PLL3_CTRL_N_MASK, CCM_PLL3_CTRL_N(mul) | CCM_PLL3_CTRL_DIV2(div2));

	if (!was_enabled) {
		/* PLL Not Enabled Yet, enable the PLL*/
		/* Following steps in User Manual Section 3.3.4.4 */

		clrsetbits_le32(&ccm->pll3_cfg, CCM_PLL3_OUT_EN, CCM_PLL3_CTRL_EN | CCM_PLL3_LDO_EN);
	}
	else {
		/* PLL was already enabled, need to clear lock enable before setting it again*/
		clrbits_le32(&ccm->pll3_cfg, CCM_PLL3_LOCK_EN);
	}

	/* Set the PLL lock enable to begin lock process */
	setbits_le32(&ccm->pll3_cfg, CCM_PLL3_LOCK_EN);

	/* Wait for lock to set to 1 */
	while (!(readl(&ccm->pll3_cfg) & CCM_PLL3_LOCK)) {}

	if (!was_enabled) {
		/* If first time enabling, delay for 20 us before enabling gate*/
		udelay(20);
	}

	/* Enable PLL Output */
	setbits_le32(&ccm->pll3_cfg, CCM_PLL3_OUT_EN);
}

unsigned int clock_get_pll6(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint32_t rval = readl(&ccm->pll6_cfg);
	int n = ((rval & CCM_PLL6_CTRL_N_MASK) >> CCM_PLL6_CTRL_N_SHIFT) + 1;
	int div2 = ((rval & CCM_PLL6_CTRL_DIV2_MASK) >>
		    CCM_PLL6_CTRL_DIV2_SHIFT) + 1;
	int div1, m;

	if (IS_ENABLED(CONFIG_SUNXI_GEN_NCAT2)) {
		div1 = ((rval & CCM_PLL6_CTRL_P0_MASK) >>
			CCM_PLL6_CTRL_P0_SHIFT) + 1;
		m = 1;
	} else {
		div1 = ((rval & CCM_PLL6_CTRL_DIV1_MASK) >>
			CCM_PLL6_CTRL_DIV1_SHIFT) + 1;
		if (IS_ENABLED(CONFIG_MACH_SUN50I_H6))
			m = 4;
		else
			m = 2;
	}

	return 24000000U * n / m / div1 / div2;
}

unsigned int clock_get_pll3(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint32_t rval = readl(&ccm->pll3_cfg);
	int n = ((rval & CCM_PLL3_CTRL_N_MASK) >> CCM_PLL3_CTRL_N_SHIFT) + 1;
	int m = ((rval & CCM_PLL3_CTRL_DIV2_MASK) >> CCM_PLL3_CTRL_DIV2_SHIFT) + 1;

	/* Multiply by 1000 after dividing by m to avoid integer overflows */
	return (24000 * n / m) * 1000;
}

int clock_twi_onoff(int port, int state)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	struct sunxi_prcm_reg *const prcm =
		(struct sunxi_prcm_reg *)SUNXI_PRCM_BASE;
	u32 value, *ptr;
	int shift;

	value = BIT(GATE_SHIFT) | BIT (RESET_SHIFT);

	if (port == 5) {
		shift = 0;
		ptr = &prcm->twi_gate_reset;
	} else {
		shift = port;
		ptr = &ccm->twi_gate_reset;
	}

	/* set the apb clock gate and reset for twi */
	if (state)
		setbits_le32(ptr, value << shift);
	else
		clrbits_le32(ptr, value << shift);

	return 0;
}
