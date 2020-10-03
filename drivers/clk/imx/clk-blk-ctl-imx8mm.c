// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Marek Vasut <marex@denx.de>
 */

#include <dt-bindings/clock/imx8mm-clock.h>
#include <dt-bindings/reset/imx8mm-reset.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "clk.h"
#include "clk-blk-ctl.h"

#define IMX_MEDIA_BLK_CTL_SFT_RSTN		0x0
#define IMX_MEDIA_BLK_CTL_CLK_EN		0x4
#define IMX_MEDIA_BLK_CTL_MIPI_RST		0x8

static struct imx_blk_ctl_hw imx8mm_dispmix_blk_ctl_hws[] = {
	/* clocks */
	IMX_BLK_CTL_CLK_GATE("lcdif_apb_clk", IMX8MM_CLK_MEDIA_BLK_CTL_LCDIF_APB, 0x4, 6, "disp_apb"),
	IMX_BLK_CTL_CLK_GATE("lcdif_pixel_clk", IMX8MM_CLK_MEDIA_BLK_CTL_LCDIF_PIXEL, 0x4, 7, "lcdif_pixel"),
	IMX_BLK_CTL_CLK_GATE("mipi_dsi_pclk", IMX8MM_CLK_MEDIA_BLK_CTL_MIPI_DSI_PCLK, 0x4, 8, "dsi_core"),
	IMX_BLK_CTL_CLK_GATE("mipi_dsi_clkref", IMX8MM_CLK_MEDIA_BLK_CTL_MIPI_DSI_CLKREF, 0x4, 9, "dsi_phy_ref"),

	/* resets */
	IMX_BLK_CTL_RESET(IMX8MM_MEDIA_BLK_CTL_RESET_MIPI_DSI_I_PRESET, 0x0, 5),
	IMX_BLK_CTL_RESET(IMX8MM_MEDIA_BLK_CTL_RESET_MIPI_M_RESET, 0x8, 17),
};

const struct imx_blk_ctl_dev_data imx8mm_dispmix_blk_ctl_dev_data __initconst = {
	.hws = imx8mm_dispmix_blk_ctl_hws,
	.hws_num = ARRAY_SIZE(imx8mm_dispmix_blk_ctl_hws),
	.clocks_max = IMX8MM_CLK_MEDIA_BLK_CTL_END,
	.resets_max = IMX8MM_MEDIA_BLK_CTL_RESET_NUM,
	.pm_runtime_saved_regs_num = 3,
	.pm_runtime_saved_regs = {
		IMX_MEDIA_BLK_CTL_SFT_RSTN,
		IMX_MEDIA_BLK_CTL_CLK_EN,
		IMX_MEDIA_BLK_CTL_MIPI_RST,
	},
};

static const struct of_device_id imx_blk_ctl_of_match[] = {
	{
		.compatible = "fsl,imx8mm-dispmix-blk-ctl",
		.data = &imx8mm_dispmix_blk_ctl_dev_data
	},
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_blk_ctl_of_match);

static int imx8mm_blk_ctl_probe(struct platform_device *pdev)
{
	return imx_blk_ctl_register(pdev);
}

static struct platform_driver imx_blk_ctl_driver = {
	.probe = imx8mm_blk_ctl_probe,
	.driver = {
		.name = "imx8mm-blk-ctl",
		.of_match_table = of_match_ptr(imx_blk_ctl_of_match),
		.pm = &imx_blk_ctl_pm_ops,
	},
};
module_platform_driver(imx_blk_ctl_driver);