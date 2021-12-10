// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for the touch controller found on a DFROBOT DFR0550/DFR0776
 * touchscreen display.
 *
 * These touchscreen displays are intended to be compatible with the official
 * Raspberry Pi 7in display which has an FTx06 touch controller directly
 * attached to the 15pin connector to the host processor. However these
 * displays have an FTx06 touch controller that connected to an I2C master
 * on a STM32F103 micro controller which polls the FTx06 and emulates a
 * virtual I2C device connected to the 15pin connector to the host processor.
 * The emulated FTx06 implements a subset of the FTx06 register set but
 * must be read with individual transactions between reading the number
 * of points and the point data itself.
 *
 * Additionally there is no IRQ made available so this is a polling driver.
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define TOUCH_EVENT_DOWN		0x00
#define TOUCH_EVENT_UP			0x01
#define TOUCH_EVENT_ON			0x02
#define TOUCH_EVENT_RESERVED		0x03

struct dfr0550_ts_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct touchscreen_properties prop;
	int max_support_points;
	unsigned int known_ids;
	u16 num_x;
	u16 num_y;
};

static int dfr0550_i2c_read(struct i2c_client *client, u8 reg, u8 *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret;

	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len = len;
	msgs[1].buf = buf;

	ret = i2c_transfer(client->adapter, msgs, 2);
		return reg < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

static void dfr0550_ts_poll(struct input_dev *input)
{
	struct dfr0550_ts_data *ts = input_get_drvdata(input);
	unsigned int active_ids = 0;
	int i, type, x, y, id, points;
	long released_ids;
	u8 buf[4];
	int error;

	error = dfr0550_i2c_read(ts->client, 0x02, buf, 1);
	if (error)
		goto error;
	/* official rpi 7in display with ft5x06 shows 0xff until touched */
	if (buf[0] == 0xff)
		return;
	points = min(buf[0] & 0xf, ts->max_support_points);

	for (i = 0; i < points; i++) {
		error = dfr0550_i2c_read(ts->client, 3+6*i, buf, 4);
		if (error)
			goto error;

		type = (buf[0] >> 6) & 0x3;
		id = (buf[2] >> 4) & 0xf;
		x = ((buf[0] & 0xf) << 8) + buf[1];
		y = ((buf[2] & 0xf) << 8) + buf[3];

		active_ids |= BIT(id);
		if (type == TOUCH_EVENT_UP ||
		    type == TOUCH_EVENT_ON) {
			input_mt_slot(input, id);
			input_mt_report_slot_state(input, MT_TOOL_FINGER, 1);
			touchscreen_report_pos(input, &ts->prop, x, y, true);
		}
	}

	released_ids = ts->known_ids & ~active_ids;
	for_each_set_bit(i, &released_ids, ts->max_support_points) {
		input_mt_slot(input, i);
		input_mt_report_slot_inactive(input);
	}
	ts->known_ids = active_ids;

	input_mt_sync_frame(input);
	input_mt_report_pointer_emulation(input, true);
	input_sync(input);

	return;

error:
	dev_err_ratelimited(&ts->client->dev,
			    "Unable to fetch data, error: %d\n", error);
}

static int dfr0550_ts_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct dfr0550_ts_data *tsdata;
	struct input_dev *input;
	u32 poll_interval = 0;
	int error;

	tsdata = devm_kzalloc(&client->dev, sizeof(*tsdata), GFP_KERNEL);
	if (!tsdata)
		return -ENOMEM;

	input = devm_input_allocate_device(&client->dev);
	if (!input) {
		dev_err(&client->dev, "failed to allocate input device.\n");
		return -ENOMEM;
	}

	tsdata->client = client;
	tsdata->input = input;

	input->name = "dfr0550-ts";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	input_set_abs_params(input, ABS_MT_POSITION_X,
			     0, tsdata->num_x * 64 - 1, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y,
			     0, tsdata->num_y * 64 - 1, 0, 0);

	touchscreen_parse_properties(input, true, &tsdata->prop);

	tsdata->max_support_points = 5;
	error = input_mt_init_slots(input, tsdata->max_support_points,
				INPUT_MT_DIRECT);
	if (error) {
		dev_err(&client->dev, "Unable to init MT slots.\n");
		return error;
	}

	i2c_set_clientdata(client, tsdata);

	device_property_read_u32(&client->dev, "poll-interval",
				 &poll_interval);
	error = input_setup_polling(input, dfr0550_ts_poll);
	if (error) {
		dev_err(&client->dev,
			"Unable to set up polling mode: %d\n", error);
		return error;
	}
	input_set_drvdata(input, tsdata);
	input_set_poll_interval(input, poll_interval);

	error = input_register_device(input);
	if (error)
		return error;

	dev_info(&client->dev, "Polling device at %dms\n", poll_interval);

	return 0;
}

static int dfr0550_ts_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id dfr0550_ts_id[] = {
	{ .name = "dfr,dfr0550" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, dfr0550_ts_id);

static const struct of_device_id dfr0550_of_match[] = {
	{ .compatible = "dfr,dfr0550", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, dfr0550_of_match);

static struct i2c_driver dfr0550_ts_driver = {
	.driver = {
		.name = "dfr0550",
		.of_match_table = dfr0550_of_match,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
	.id_table = dfr0550_ts_id,
	.probe    = dfr0550_ts_probe,
	.remove   = dfr0550_ts_remove,
};

module_i2c_driver(dfr0550_ts_driver);

MODULE_AUTHOR("Tim Harvey <tharvey@gateworks.com>");
MODULE_DESCRIPTION("DFROBOT DFR0550 I2C Touchscreen Driver");
MODULE_LICENSE("GPL v2");
