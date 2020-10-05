// SPDX-License-Identifier: GPL-2.0-only
/*
 * NXP i.MX8M SoC MIPI DSI driver
 *
 * Copyright (C) 2020 Marek Vasut <marex@denx.de>
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <drm/bridge/samsung-dsim.h>
#include <drm/drm_bridge.h>
#include <drm/drm_encoder.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

enum {
	DSI_PORT_IN,
	DSI_PORT_OUT
};

struct imx_dsim_priv {
	struct samsung_dsim *dsi;
	struct drm_encoder encoder;
};

static const unsigned int imx8mm_dsim_reg_values[] = {
	[RESET_TYPE] = DSIM_SWRST,
	[PLL_TIMER] = 500,
	[STOP_STATE_CNT] = 0xf,
	[PHYCTRL_ULPS_EXIT] = 0xaf,
	[PHYCTRL_VREG_LP] = 0,
	[PHYCTRL_SLEW_UP] = 0,
	[PHYTIMING_LPX] = 0x06,
	[PHYTIMING_HS_EXIT] = 0x0b,
	[PHYTIMING_CLK_PREPARE] = 0x07,
	[PHYTIMING_CLK_ZERO] = 0x26,
	[PHYTIMING_CLK_POST] = 0x0d,
	[PHYTIMING_CLK_TRAIL] = 0x08,
	[PHYTIMING_HS_PREPARE] = 0x08,
	[PHYTIMING_HS_ZERO] = 0x0d,
	[PHYTIMING_HS_TRAIL] = 0x0b,
};

static int imx_dsim_host_attach(struct device *dev,
				  struct mipi_dsi_device *device)
{
	struct imx_dsim_priv *dsi = dev_get_drvdata(dev);
	struct drm_device *drm = dsi->encoder.dev;

	if (drm->mode_config.poll_enabled)
		drm_kms_helper_hotplug_event(drm);

	return 0;
}

static int imx_dsim_host_detach(struct device *dev,
				  struct mipi_dsi_device *device)
{
	struct imx_dsim_priv *dsi = dev_get_drvdata(dev);
	struct drm_device *drm = dsi->encoder.dev;

	if (drm->mode_config.poll_enabled)
		drm_kms_helper_hotplug_event(drm);

	return 0;
}

static const struct samsung_dsim_host_ops imx_dsim_host_ops = {
	.attach = imx_dsim_host_attach,
	.detach = imx_dsim_host_detach,
};

static const struct samsung_dsim_driver_data imx8mm_dsi_driver_data = {
	.reg_ofs = EXYNOS5433_REG_OFS,
	.plltmr_reg = 0xa0,
	.has_clklane_stop = 1,
	.num_clks = 2,
	.max_freq = 2100,
	.wait_for_reset = 0,
	.num_bits_resol = 12,
	.reg_values = imx8mm_dsim_reg_values,
	.host_ops = &imx_dsim_host_ops,
};

static const struct of_device_id imx_dsim_of_match[] = {
	{ .compatible = "fsl,imx8mm-mipi-dsim",
	  .data = &imx8mm_dsi_driver_data },
	{ }
};

static int imx_dsim_probe(struct platform_device *pdev)
{
	struct imx_dsim_priv *dsi;
	struct device *dev = &pdev->dev;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;
	platform_set_drvdata(pdev, dsi);

	dsi->dsi = samsung_dsim_probe(pdev);
	if (IS_ERR(dsi->dsi))
		return PTR_ERR(dsi->dsi);

	pm_runtime_enable(dev);

	return 0;
}

static int imx_dsim_remove(struct platform_device *pdev)
{
	struct imx_dsim_priv *dsi = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);

	samsung_dsim_remove(dsi->dsi);

	return 0;
}

static int __maybe_unused imx_dsim_suspend(struct device *dev)
{
	struct imx_dsim_priv *dsi = dev_get_drvdata(dev);

	return samsung_dsim_suspend(dsi->dsi);
}

static int __maybe_unused imx_dsim_resume(struct device *dev)
{
	struct imx_dsim_priv *dsi = dev_get_drvdata(dev);

	return samsung_dsim_resume(dsi->dsi);
}

static const struct dev_pm_ops imx_dsim_pm_ops = {
	SET_RUNTIME_PM_OPS(imx_dsim_suspend, imx_dsim_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static struct platform_driver imx_dsim_driver = {
	.probe = imx_dsim_probe,
	.remove = imx_dsim_remove,
	.driver = {
		   .name = "imx-dsim-dsi",
		   .owner = THIS_MODULE,
		   .pm = &imx_dsim_pm_ops,
		   .of_match_table = imx_dsim_of_match,
	},
};

module_platform_driver(imx_dsim_driver);

MODULE_AUTHOR("Marek Vasut <marex@denx.de>");
MODULE_DESCRIPTION("NXP i.MX8M SoC MIPI DSI");
MODULE_LICENSE("GPL v2");
