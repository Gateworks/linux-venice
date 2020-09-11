/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __EXYNOS_DRM_DSI__
#define __EXYNOS_DRM_DSI__

struct drm_encoder;
struct samsung_dsim;
struct platform_device;
struct mipi_dsi_device;

enum exynos_reg_offset {
	EXYNOS_REG_OFS,
	EXYNOS5433_REG_OFS
};

struct samsung_dsim *samsung_dsim_probe(struct platform_device *pdev);
void samsung_dsim_remove(struct samsung_dsim *dsi);
int samsung_dsim_bind(struct samsung_dsim *dsi, struct drm_encoder *encoder);
void samsung_dsim_unbind(struct samsung_dsim *dsi);

int samsung_dsim_suspend(struct samsung_dsim *dsi);
int samsung_dsim_resume(struct samsung_dsim *dsi);

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

struct samsung_dsim_host_ops {
	int (*attach)(struct device *dev, struct mipi_dsi_device *device);
	int (*detach)(struct device *dev, struct mipi_dsi_device *device);
	void (*te_handler)(struct device *dev);
};

struct samsung_dsim_driver_data {
	enum exynos_reg_offset reg_ofs;
	unsigned int plltmr_reg;
	unsigned int has_freqband:1;
	unsigned int has_clklane_stop:1;
	unsigned int num_clks;
	unsigned int max_freq;
	unsigned int wait_for_reset;
	unsigned int num_bits_resol;
	const unsigned int *reg_values;
	const struct samsung_dsim_host_ops *host_ops;
};

#endif /* __EXYNOS_DRM_DSI__ */
