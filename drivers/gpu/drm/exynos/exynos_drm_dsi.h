/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __EXYNOS_DRM_DSI__
#define __EXYNOS_DRM_DSI__

struct drm_encoder;
struct exynos_dsi;
struct platform_device;
struct mipi_dsi_device;

enum exynos_reg_offset {
	EXYNOS_REG_OFS,
	EXYNOS5433_REG_OFS
};

struct exynos_dsi *exynos_dsi_probe(struct platform_device *pdev);
void exynos_dsi_remove(struct exynos_dsi *dsi);
int exynos_dsi_bind(struct exynos_dsi *dsi, struct drm_encoder *encoder);
void exynos_dsi_unbind(struct exynos_dsi *dsi);

int exynos_dsi_suspend(struct exynos_dsi *dsi);
int exynos_dsi_resume(struct exynos_dsi *dsi);

enum reg_value_idx {
	RESET_TYPE,
	PLL_TIMER,
	STOP_STATE_CNT,
	PHYCTRL_ULPS_EXIT,
	PHYCTRL_VREG_LP,
	PHYCTRL_SLEW_UP,
	PHYTIMING_LPX,
	PHYTIMING_HS_EXIT,
	PHYTIMING_CLK_PREPARE,
	PHYTIMING_CLK_ZERO,
	PHYTIMING_CLK_POST,
	PHYTIMING_CLK_TRAIL,
	PHYTIMING_HS_PREPARE,
	PHYTIMING_HS_ZERO,
	PHYTIMING_HS_TRAIL
};

/* DSIM_SWRST */
#define DSIM_FUNCRST			(1 << 16)
#define DSIM_SWRST			(1 << 0)

struct exynos_dsi_host_ops {
	int (*attach)(struct device *dev, struct mipi_dsi_device *device);
	int (*detach)(struct device *dev, struct mipi_dsi_device *device);
	void (*te_handler)(struct device *dev);
};

struct exynos_dsi_driver_data {
	enum exynos_reg_offset reg_ofs;
	unsigned int plltmr_reg;
	unsigned int has_freqband:1;
	unsigned int has_clklane_stop:1;
	unsigned int num_clks;
	unsigned int max_freq;
	unsigned int wait_for_reset;
	unsigned int num_bits_resol;
	const unsigned int *reg_values;
	const struct exynos_dsi_host_ops *host_ops;
};

#endif /* __EXYNOS_DRM_DSI__ */
