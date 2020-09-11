// SPDX-License-Identifier: GPL-2.0-only
/*
 * Samsung SoC MIPI DSI Master driver.
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * Contacts: Tomasz Figa <t.figa@samsung.com>
 */

#include <linux/component.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/pm_runtime.h>

#include <drm/bridge/samsung-dsim.h>
#include <drm/drm_bridge.h>
#include <drm/drm_encoder.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "exynos_drm_crtc.h"
#include "exynos_drm_drv.h"

enum {
	DSI_PORT_IN,
	DSI_PORT_OUT
};

struct exynos_dsi {
	struct samsung_dsim *dsi;
	struct drm_encoder encoder;
};

static const unsigned int reg_values[] = {
	[RESET_TYPE] = DSIM_SWRST,
	[PLL_TIMER] = 500,
	[STOP_STATE_CNT] = 0xf,
	[PHYCTRL_ULPS_EXIT] = 0x0af,
	[PHYCTRL_VREG_LP] = 0,
	[PHYCTRL_SLEW_UP] = 0,
	[PHYTIMING_LPX] = 0x06,
	[PHYTIMING_HS_EXIT] = 0x0b,
	[PHYTIMING_CLK_PREPARE] = 0x07,
	[PHYTIMING_CLK_ZERO] = 0x27,
	[PHYTIMING_CLK_POST] = 0x0d,
	[PHYTIMING_CLK_TRAIL] = 0x08,
	[PHYTIMING_HS_PREPARE] = 0x09,
	[PHYTIMING_HS_ZERO] = 0x0d,
	[PHYTIMING_HS_TRAIL] = 0x0b,
};

static const unsigned int exynos5422_reg_values[] = {
	[RESET_TYPE] = DSIM_SWRST,
	[PLL_TIMER] = 500,
	[STOP_STATE_CNT] = 0xf,
	[PHYCTRL_ULPS_EXIT] = 0xaf,
	[PHYCTRL_VREG_LP] = 0,
	[PHYCTRL_SLEW_UP] = 0,
	[PHYTIMING_LPX] = 0x08,
	[PHYTIMING_HS_EXIT] = 0x0d,
	[PHYTIMING_CLK_PREPARE] = 0x09,
	[PHYTIMING_CLK_ZERO] = 0x30,
	[PHYTIMING_CLK_POST] = 0x0e,
	[PHYTIMING_CLK_TRAIL] = 0x0a,
	[PHYTIMING_HS_PREPARE] = 0x0c,
	[PHYTIMING_HS_ZERO] = 0x11,
	[PHYTIMING_HS_TRAIL] = 0x0d,
};

static const unsigned int exynos5433_reg_values[] = {
	[RESET_TYPE] = DSIM_FUNCRST,
	[PLL_TIMER] = 22200,
	[STOP_STATE_CNT] = 0xa,
	[PHYCTRL_ULPS_EXIT] = 0x190,
	[PHYCTRL_VREG_LP] = 1,
	[PHYCTRL_SLEW_UP] = 1,
	[PHYTIMING_LPX] = 0x07,
	[PHYTIMING_HS_EXIT] = 0x0c,
	[PHYTIMING_CLK_PREPARE] = 0x09,
	[PHYTIMING_CLK_ZERO] = 0x2d,
	[PHYTIMING_CLK_POST] = 0x0e,
	[PHYTIMING_CLK_TRAIL] = 0x09,
	[PHYTIMING_HS_PREPARE] = 0x0b,
	[PHYTIMING_HS_ZERO] = 0x10,
	[PHYTIMING_HS_TRAIL] = 0x0c,
};

static int exynos_dsi_host_attach(struct device *dev,
				  struct mipi_dsi_device *device)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);
	struct drm_device *drm = dsi->encoder.dev;
	struct exynos_drm_crtc *crtc;

	mutex_lock(&drm->mode_config.mutex);
	crtc = exynos_drm_crtc_get_by_type(drm, EXYNOS_DISPLAY_TYPE_LCD);
	crtc->i80_mode = !(device->mode_flags & MIPI_DSI_MODE_VIDEO);
	mutex_unlock(&drm->mode_config.mutex);

	if (drm->mode_config.poll_enabled)
		drm_kms_helper_hotplug_event(drm);

	return 0;
}

static int exynos_dsi_host_detach(struct device *dev,
				  struct mipi_dsi_device *device)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);
	struct drm_device *drm = dsi->encoder.dev;

	if (drm->mode_config.poll_enabled)
		drm_kms_helper_hotplug_event(drm);

	return 0;
}

static void exynos_dsi_te_handler(struct device *dev)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);

	exynos_drm_crtc_te_handler(dsi->encoder.crtc);
}

static const struct samsung_dsim_host_ops exynos_dsi_host_ops = {
	.attach = exynos_dsi_host_attach,
	.detach = exynos_dsi_host_detach,
	.te_handler = exynos_dsi_te_handler,
};

static const struct samsung_dsim_driver_data exynos3_dsi_driver_data = {
	.reg_ofs = EXYNOS_REG_OFS,
	.plltmr_reg = 0x50,
	.has_freqband = 1,
	.has_clklane_stop = 1,
	.num_clks = 2,
	.max_freq = 1000,
	.wait_for_reset = 1,
	.num_bits_resol = 11,
	.reg_values = reg_values,
	.host_ops = &exynos_dsi_host_ops,
};

static const struct samsung_dsim_driver_data exynos4_dsi_driver_data = {
	.reg_ofs = EXYNOS_REG_OFS,
	.plltmr_reg = 0x50,
	.has_freqband = 1,
	.has_clklane_stop = 1,
	.num_clks = 2,
	.max_freq = 1000,
	.wait_for_reset = 1,
	.num_bits_resol = 11,
	.reg_values = reg_values,
	.host_ops = &exynos_dsi_host_ops,
};

static const struct samsung_dsim_driver_data exynos5_dsi_driver_data = {
	.reg_ofs = EXYNOS_REG_OFS,
	.plltmr_reg = 0x58,
	.num_clks = 2,
	.max_freq = 1000,
	.wait_for_reset = 1,
	.num_bits_resol = 11,
	.reg_values = reg_values,
	.host_ops = &exynos_dsi_host_ops,
};

static const struct samsung_dsim_driver_data exynos5433_dsi_driver_data = {
	.reg_ofs = EXYNOS5433_REG_OFS,
	.plltmr_reg = 0xa0,
	.has_clklane_stop = 1,
	.num_clks = 5,
	.max_freq = 1500,
	.wait_for_reset = 0,
	.num_bits_resol = 12,
	.reg_values = exynos5433_reg_values,
	.host_ops = &exynos_dsi_host_ops,
};

static const struct samsung_dsim_driver_data exynos5422_dsi_driver_data = {
	.reg_ofs = EXYNOS5433_REG_OFS,
	.plltmr_reg = 0xa0,
	.has_clklane_stop = 1,
	.num_clks = 2,
	.max_freq = 1500,
	.wait_for_reset = 1,
	.num_bits_resol = 12,
	.reg_values = exynos5422_reg_values,
	.host_ops = &exynos_dsi_host_ops,
};

static const struct of_device_id exynos_dsi_of_match[] = {
	{ .compatible = "samsung,exynos3250-mipi-dsi",
	  .data = &exynos3_dsi_driver_data },
	{ .compatible = "samsung,exynos4210-mipi-dsi",
	  .data = &exynos4_dsi_driver_data },
	{ .compatible = "samsung,exynos5410-mipi-dsi",
	  .data = &exynos5_dsi_driver_data },
	{ .compatible = "samsung,exynos5422-mipi-dsi",
	  .data = &exynos5422_dsi_driver_data },
	{ .compatible = "samsung,exynos5433-mipi-dsi",
	  .data = &exynos5433_dsi_driver_data },
	{ }
};

static int exynos_dsi_bind(struct device *dev,
			   struct device *master, void *data)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);
	struct drm_encoder *encoder = &dsi->encoder;
	struct drm_device *drm_dev = data;
	struct device_node *in_bridge_node;
	struct drm_bridge *in_bridge;
	int ret;

	drm_simple_encoder_init(drm_dev, encoder, DRM_MODE_ENCODER_TMDS);

	ret = exynos_drm_set_possible_crtcs(encoder, EXYNOS_DISPLAY_TYPE_LCD);
	if (ret < 0)
		return ret;

	in_bridge_node = of_graph_get_remote_node(dev->of_node, DSI_PORT_IN, 0);
	if (in_bridge_node) {
		in_bridge = of_drm_find_bridge(in_bridge_node);
		if (in_bridge)
			drm_bridge_attach(encoder, in_bridge, NULL, 0);
		of_node_put(in_bridge_node);
	}

	ret = samsung_dsim_bind(dsi->dsi, encoder);
	if (ret)
		goto err;

	return 0;

err:
	drm_encoder_cleanup(encoder);
	return ret;
}

static void exynos_dsi_unbind(struct device *dev,
			      struct device *master, void *data)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);
	struct drm_encoder *encoder = &dsi->encoder;

	samsung_dsim_unbind(dsi->dsi);

	drm_encoder_cleanup(encoder);
}

static const struct component_ops exynos_dsi_component_ops = {
	.bind	= exynos_dsi_bind,
	.unbind	= exynos_dsi_unbind,
};

static int exynos_dsi_probe(struct platform_device *pdev)
{
	struct exynos_dsi *dsi;
	struct device *dev = &pdev->dev;
	int ret;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;
	platform_set_drvdata(pdev, dsi);

	dsi->dsi = samsung_dsim_probe(pdev);
	if (IS_ERR(dsi->dsi))
		return PTR_ERR(dsi->dsi);

	pm_runtime_enable(dev);

	ret = component_add(dev, &exynos_dsi_component_ops);
	if (ret)
		goto err_disable_runtime;

	return 0;

err_disable_runtime:
	pm_runtime_disable(dev);

	return ret;
}

static int exynos_dsi_remove(struct platform_device *pdev)
{
	struct exynos_dsi *dsi = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);

	samsung_dsim_remove(dsi->dsi);

	component_del(&pdev->dev, &exynos_dsi_component_ops);

	return 0;
}

static int __maybe_unused exynos_dsi_suspend(struct device *dev)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);

	return samsung_dsim_suspend(dsi->dsi);
}

static int __maybe_unused exynos_dsi_resume(struct device *dev)
{
	struct exynos_dsi *dsi = dev_get_drvdata(dev);

	return samsung_dsim_resume(dsi->dsi);
}

static const struct dev_pm_ops exynos_dsi_pm_ops = {
	SET_RUNTIME_PM_OPS(exynos_dsi_suspend, exynos_dsi_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

struct platform_driver dsi_driver = {
	.probe = exynos_dsi_probe,
	.remove = exynos_dsi_remove,
	.driver = {
		   .name = "exynos-dsi",
		   .owner = THIS_MODULE,
		   .pm = &exynos_dsi_pm_ops,
		   .of_match_table = exynos_dsi_of_match,
	},
};

MODULE_AUTHOR("Tomasz Figa <t.figa@samsung.com>");
MODULE_AUTHOR("Andrzej Hajda <a.hajda@samsung.com>");
MODULE_DESCRIPTION("Samsung SoC MIPI DSI Master");
MODULE_LICENSE("GPL v2");
