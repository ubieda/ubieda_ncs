#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/cx_endpoint.h>

#include <settings/settings.h>

#include <dk_buttons_and_leds.h>

#include <motor_controller.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  100

#define USER_LED                DK_LED3

#define USER_BUTTON             DK_BTN1_MSK

static bool app_button_state;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CX_ENDPOINT_VAL),
};

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err %u)", err);
		return;
	}

	LOG_INF("Connected");

	//dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)", reason);

	int err = motor_controller_set(0);
	LOG_INF("Motor Controller Speed set to %d - Result: %d",0,err);

	//dk_set_led_off(CON_STATUS_LED);
}

static struct bt_conn_cb conn_callbacks = {
	.connected        = connected,
	.disconnected     = disconnected,
};

static struct bt_conn_auth_cb conn_auth_callbacks;

static void recv_data_cb(const uint8_t *data, uint16_t len)
{
	int err;
	LOG_INF("received data - Len: %d",len);
	LOG_HEXDUMP_INF(data,len,"recvd_data");

	int8_t speed = data[0];
	err = motor_controller_set(speed);
	LOG_INF("Motor Controller Speed set to %d - Result: %d",speed,err);
	if(err)
		speed = 0;

	err = bt_cx_endpoint_send_data(&speed,1);
	LOG_INF("Sending back the speed set - Result: %d",err);

	//dk_set_led(USER_LED, data[0]);
}

static struct bt_cx_endpoint_cb cx_endpoint_callbacs = {
	.recv_cb    = recv_data_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		bt_cx_endpoint_send_data((const uint8_t *)&button_state,1);
		app_button_state = button_state ? true : false;
	}
}

static int init_button(void)
{
	int err;

	// err = dk_buttons_init(button_changed);
	// if (err) {
	// 	LOG_INF("Cannot init buttons (err: %d)", err);
	// }

	return err;
}

void main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting BLE Motor Controller Application!");

	// err = dk_leds_init();
	// if (err) {
	// 	LOG_INF("LEDs init failed (err %d)", err);
	// 	return;
	// }

	err = motor_controller_init();
	if (err) {
		LOG_INF("Motor Ctlr init failed (err %d)", err);
		return;
	}

	err = init_button();
	if (err) {
		LOG_INF("Button init failed (err %d)", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	if (IS_ENABLED(CONFIG_BT_CX_ENDPOINT_SECURITY_ENABLED)) {
		bt_conn_auth_cb_register(&conn_auth_callbacks);
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_cx_endpoint_init(&cx_endpoint_callbacs);
	if (err) {
		LOG_INF("Failed to init CX_ENDPOINT (err:%d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_INF("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising successfully started");

	for (;;) {
		//dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
