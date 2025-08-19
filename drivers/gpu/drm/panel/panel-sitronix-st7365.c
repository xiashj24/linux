/*
 * Copyright (C) 2025, Korg Inc.
 * Author: Shijie Xia
 */

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/media-bus-format.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>

#include <video/mipi_display.h>

struct st7365_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	void (*init_sequence)(struct mipi_dsi_multi_context *ctx);
};

struct st7365 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct st7365_panel_desc *desc;

	struct gpio_desc *reset;
	enum drm_panel_orientation orientation;
};

static inline struct st7365 *panel_to_st7365(struct drm_panel *panel)
{
	return container_of(panel, struct st7365, panel);
}

static void gd035hv316b_init_sequence(struct mipi_dsi_multi_context *ctx)
{
	mipi_dsi_dcs_write_seq_multi(ctx, 0x11);
	mipi_dsi_msleep(ctx, 120);

	// Memory Data Access Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0x36, 0x48);
	// Interface Pixel Format
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3A, 0x66);
	// Command Control (enable command 2)
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0xC3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x96);
	// Interface Mode Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB0, 0x80);
	// Frame Rate Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB1, 0x80, 0x10);
	// Display Inversion Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB4, 0x02); // 2-dot inversion
	// Blanking Porch Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB5, 0x1F, 0x50, 0x00, 0x20);
	// Display Function Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB6, 0x20, 0x02, 0x3B);
	// Entry Mode Set
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB7, 0xC6);
	// Power Control 1
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC0, 0xC0, 0x25);
	// Power Control 2
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC1, 0x09);
	// Power Control 3
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC2, 0xA7);
	// VCOM Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC5, 0x1D);
	// Display Output Ctrl Adjust
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE8, 0x40, 0x8A, 0x1B, 0x1B, 0x23,
				     0x0A, 0xAC, 0x33);
	// Positive Gamma Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE0, 0xD2, 0x05, 0x08, 0x06, 0x05,
				     0x02, 0x2A, 0x44, 0x46, 0x39, 0x15, 0x15,
				     0x2D, 0x32);
	// Negative Gamma Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE1, 0x96, 0x08, 0x0C, 0x09, 0x09,
				     0x25, 0x2E, 0x43, 0x42, 0x35, 0x11, 0x11,
				     0x28, 0x2E);

	// Command Control (disable command 2)
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x3C);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x69);

	mipi_dsi_msleep(ctx, 120);
	mipi_dsi_dcs_write_seq_multi(ctx, 0x21); // Inversion On
}

// static void gd035hv316b_init_sequence_2(struct mipi_dsi_multi_context *ctx)
// {
// 	// Sleep Out
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0x11);
// 	mipi_dsi_msleep(ctx, 120);

// 	// Memory Data Access Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0x36, 0x48);
// 	// Interface Pixel Format
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0x3A, 0x56);
// 	// Command Set Control (enable command 2)
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0xC3);
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x96);
// 	// Interface Mode Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB0, 0x80);
// 	// Frame Rate Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB1, 0x80, 0x10);
// 	// Display Inversion Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB4, 0x01); // 0x01: 1-dot inversion
// 	// Blanking Porch Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB5, 0x1F, 0x50, 0x00, 0x20);
// 	// Display Function Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB6, 0x20, 0x02, 0x3B);
// 	// Entry Mode Set
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB7, 0xC6);
// 	// Mode Selection
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xB9, 0x02); // dithering on
// 	// Power Control 1
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xC0, 0x80, 0x25);
// 	// Power Control 2
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xC1, 0x09);
// 	// Power Control 3
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xC2, 0xA7);
// 	// VCOM Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xC5, 0x0F);
// 	// Display Output Ctrl Adjust
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29,
// 				     0x19, 0xA5, 0x33);

// 	// Positive Gamma Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xE0, 0xD2, 0x05, 0x08, 0x06, 0x05,
// 				     0x02, 0x2A, 0x44, 0x46, 0x39, 0x15, 0x15,
// 				     0x2D, 0x32);

// 	// Negative Gamma Control
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xE1, 0x96, 0x08, 0x0C, 0x09, 0x09,
// 				     0x25, 0x2E, 0x43, 0x42, 0x35, 0x11, 0x11,
// 				     0x28, 0x2E);

// 	// Command Set Control (disable command 2)
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x3C);
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x69);

// 	mipi_dsi_msleep(ctx, 120);
// 	mipi_dsi_dcs_write_seq_multi(ctx, 0x21); // inversion on
// }

// MARK: init seq
static void jt60849_init_sequence(struct mipi_dsi_multi_context *ctx)
{
	mipi_dsi_dcs_write_seq_multi(ctx, 0x11);
	mipi_dsi_msleep(ctx, 120);

	// Memory Data Access Control
	// TODO: try row colomn exchange
	mipi_dsi_dcs_write_seq_multi(ctx, 0x36, 0x48);
	// Interface Pixel Format
	mipi_dsi_dcs_write_seq_multi(ctx, 0x3A, 0x55);
	// Command Set Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0xC3);
	mipi_dsi_dcs_write_seq_multi(ctx, 0xF0, 0x96);
	// Interface Mode Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB0, 0x80);
	// Frame Rate Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB1, 0x80, 0x10);
	// Display Inversion Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB4, 0x01); // 1-dot inversion
	// Display Function Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB6, 0x20, 0x02, 0x3B);
	// Entry Mode Set
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB7, 0xC6);
	// Mode Selection
	mipi_dsi_dcs_write_seq_multi(ctx, 0xB9, 0x02); // dithering on
	// Power Control 1
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC0, 0x80, 0x07);
	// Power Control 2
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC1, 0x09);
	// Power Control 3
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC2, 0xA7);
	// VCOM Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xC5, 0x13);
	// Display Output Control Adjust
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29,
				     0x1C, 0xAA, 0x33);
	// Positive Gamma Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE0, 0xD2, 0x05, 0x08, 0x06, 0x05,
				     0x02, 0x2A, 0x44, 0x46, 0x39, 0x15, 0x15,
				     0x2D, 0x32);
	// Negative Gamma Control
	mipi_dsi_dcs_write_seq_multi(ctx, 0xE1, 0x96, 0x08, 0x0C, 0x09, 0x09,
				     0x25, 0x2E, 0x43, 0x42, 0x35, 0x11, 0x11,
				     0x28, 0x2E);

	mipi_dsi_msleep(ctx, 120); // Delay
	mipi_dsi_dcs_write_seq_multi(ctx, 0x21); // Inversion On
	mipi_dsi_dcs_write_seq_multi(ctx, 0x29); // Display On
}

static int st7365_hard_reset(struct st7365 *ctx)
{
	gpiod_set_value(ctx->reset, 1);
	msleep(1);
	gpiod_set_value(ctx->reset, 0);
	msleep(10);
	gpiod_set_value(ctx->reset, 1);
	msleep(120);

	return 0;
}

// MARK: st7365_init
static int st7365_init(struct st7365 *panel)
{
	struct mipi_dsi_multi_context ctx = { .dsi = panel->dsi };

	// sleep off before init sequence
	mipi_dsi_dcs_exit_sleep_mode_multi(&ctx);
	mipi_dsi_msleep(&ctx, 120);

	if (panel->desc->init_sequence)
		panel->desc->init_sequence(&ctx);

	mipi_dsi_dcs_exit_sleep_mode_multi(&ctx);
	mipi_dsi_msleep(&ctx, 120);

	return ctx.accum_err;
}

static int st7365_off(struct st7365 *panel)
{
	struct mipi_dsi_multi_context ctx = { .dsi = panel->dsi };

	mipi_dsi_dcs_set_display_off_multi(&ctx);
	mipi_dsi_dcs_enter_sleep_mode_multi(&ctx);
	mipi_dsi_msleep(&ctx, 120);

	return ctx.accum_err;
}

static int st7365_prepare(struct drm_panel *panel)
{
	struct st7365 *ctx = panel_to_st7365(panel);
	struct device *dev = &(ctx->dsi->dev);
	int ret;

	dev_info(dev, "prepare start.");

	ret = st7365_hard_reset(ctx);
	if (ret < 0)
		return ret;

	ret = st7365_init(ctx);
	if (ret < 0) {
		gpiod_set_value_cansleep(ctx->reset, 0);

		dev_info(dev, "prepare failed.");
		return ret;
	}

	dev_info(dev, "prepare done.");

	return 0;
}

static int st7365_unprepare(struct drm_panel *panel)
{
	struct st7365 *ctx = panel_to_st7365(panel);
	struct device *dev = &(ctx->dsi->dev);
	int ret;

	dev_info(dev, "unprepare start.");

	ret = st7365_off(ctx);
	if (ret < 0)
		return ret;

	gpiod_set_value_cansleep(ctx->reset, 0);

	dev_info(dev, "unprepare done.");
	return 0;
}

static int st7365_enable(struct drm_panel *panel)
{
	struct st7365 *st7365 = panel_to_st7365(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = st7365->dsi };
	unsigned char pixel_format;

	dev_info(panel->dev, "enable start.");

	mipi_dsi_dcs_set_display_on_multi(&ctx);

	// mipi_dsi_dcs_get_pixel_format(st7365->dsi, &pixel_format);
	// dev_info(panel->dev, "pixel format: %d", pixel_format);

	dev_info(panel->dev, "enable done.");

	return ctx.accum_err;
}

static int st7365_disable(struct drm_panel *panel)
{
	struct st7365 *st7365 = panel_to_st7365(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = st7365->dsi };

	dev_info(panel->dev, "disable start.");

	mipi_dsi_dcs_set_display_off_multi(&ctx);

	dev_info(panel->dev, "disable done.");
	return ctx.accum_err;
}

static int st7365_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	struct st7365 *ctx = panel_to_st7365(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, ctx->desc->mode);
	if (!mode) {
		dev_err(panel->dev,
			"Failed to duplicate mode " DRM_MODE_FMT "\n",
			DRM_MODE_ARG(ctx->desc->mode));
		return 0;
	}

	if (mode->name[0] == '\0')
		drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static enum drm_panel_orientation
st7365_get_orientation(struct drm_panel *panel)
{
	struct st7365 *ctx = panel_to_st7365(panel);

	return ctx->orientation;
}

static const struct drm_panel_funcs st7365_funcs = {
	.unprepare = st7365_unprepare,
	.prepare = st7365_prepare,
	.enable = st7365_enable,
	.disable = st7365_disable,
	.get_modes = st7365_get_modes,
	.get_orientation = st7365_get_orientation,
};

// TODO: set backlight brightness
// just call mipi_dsi_dcs_set_display_brightness_multi?

// static int rpi_dsi_display_set_brightness(struct backlight_device *bl)
// {
// 	struct rpi_dsi_display *rpi_dsi_display = bl_get_data(bl);
// 	struct mipi_dsi_device *dsi = rpi_dsi_display->dsi;
// 	uint8_t brightness = bl->props.brightness;
// 	int ret = 0;
// 	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_SET_DISPLAY_BRIGHTNESS,
// 				 &brightness, sizeof(brightness));
// 	if (ret < 0)
// 		return ret;
// 	return 0;
// }

// static const struct backlight_ops rpi_dsi_display_bl_ops = {
// 	.update_status = rpi_dsi_display_set_brightness,
// };

// MARK: panel mode
static const struct drm_display_mode gd035hv316b_mode = {
	.clock = 11000,

	.hdisplay = 320,
	.hsync_start = 320 + 60, // front 60
	.hsync_end = 320 + 60 + 40, // sync 40
	.htotal = 320 + 60 + 40 + 60, // back 60

	.vdisplay = 480,
	.vsync_start = 480 + 20, // front 20
	.vsync_end = 480 + 20 + 4, // sync 4
	.vtotal = 480 + 20 + 4 + 20, // back 20

	.width_mm = 49,
	.height_mm = 73,

	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static const struct st7365_panel_desc gd035hv316b_desc = {
	.mode = &gd035hv316b_mode,
	.lanes = 1,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_MODE_LPM,
	.format = MIPI_DSI_FMT_RGB666, // MIPI_DSI_FMT_RGB888
	.init_sequence = gd035hv316b_init_sequence,
};

// MARK: desc
static const struct drm_display_mode jt60849_mode = {
	.clock = 11000,

	.hdisplay = 320,
	.hsync_start = 320 + 14, // front 14
	.hsync_end = 320 + 14 + 10, // sync 10
	.htotal = 320 + 14 + 10 + 20, // back 20

	.vdisplay = 480,
	.vsync_start = 480 + 12, // front 12
	.vsync_end = 480 + 12 + 2, // sync 2
	.vtotal = 480 + 12 + 2 + 11, // back 11

	.width_mm = 49,
	.height_mm = 73,

	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static const struct st7365_panel_desc jt60849_desc = {
	.mode = &jt60849_mode,
	.lanes = 1,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		      MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_MODE_LPM,
	.format =
		MIPI_DSI_FMT_RGB565, // MIPI_DSI_FMT_RGB666, MIPI_DSI_FMT_RGB666_PACKED, MIPI_DSI_FMT_RGB888

	.init_sequence = jt60849_init_sequence,
};

static int st7365_dsi_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct st7365 *ctx;
	int ret;

	dev_info(dev, "probe start.");

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->dsi = dsi;

	ctx->desc = of_device_get_match_data(dev);

	ctx->panel.prepare_prev_first = true;
	drm_panel_init(&ctx->panel, dev, &st7365_funcs, DRM_MODE_CONNECTOR_DSI);

	ctx->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset))
		return dev_err_probe(dev, PTR_ERR(ctx->reset),
				     "Failed get reset GPIO\n");

	ret = of_drm_get_panel_orientation(dev->of_node, &ctx->orientation);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get orientation\n");

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	dsi->mode_flags = ctx->desc->mode_flags;
	dsi->format = ctx->desc->format;
	dsi->lanes = ctx->desc->lanes;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	dev_info(dev, "probe done.");

	return 0;
}

static void st7365_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct st7365 *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id st7365_dsi_of_match[] = {
	{ .compatible = "gd035hv316b", .data = &gd035hv316b_desc },
	{ .compatible = "jt60849", .data = &jt60849_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, st7365_dsi_of_match);

static struct mipi_dsi_driver st7365_dsi_driver = {
	.probe		= st7365_dsi_probe,
	.remove		= st7365_dsi_remove,
	.driver = {
		.name		= "st7365",
		.of_match_table	= st7365_dsi_of_match,
	},
};

module_mipi_dsi_driver(st7365_dsi_driver);

MODULE_AUTHOR("Shijie Xia <xiashj@korg.co.jp>");
MODULE_DESCRIPTION("Sitronix ST7365P LCD Panel Driver");
MODULE_LICENSE("GPL");
