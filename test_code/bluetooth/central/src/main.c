/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/cx_endpoint.h>
#include <bluetooth/services/cx_endpoint_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <settings/settings.h>

#include <drivers/uart.h>

#include <logging/log.h>

#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define BTN_STATUS_LED          DK_LED3
#define RUN_LED_BLINK_INTERVAL  100

#define BUTTON_SCAN DK_BTN1_MSK
#define BUTTON_SEND_DATA DK_BTN2_MSK

#define DEVICE_NAME_FILTER      "CX-Peripheral"

static struct bt_conn *default_conn;
static struct bt_cx_endpoint_client cx_endpoint_client;

static uint8_t ble_data_received(const uint8_t *const data, uint16_t len)
{
	int err;

	LOG_INF("Received data - len: %d",len);
	LOG_HEXDUMP_INF(data,len,"received_data");

	return BT_GATT_ITER_CONTINUE;
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_cx_endpoint_client *cx_endpoint = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_cx_endpoint_handles_assign(dm, cx_endpoint);
	bt_cx_endpoint_subscribe_receive(cx_endpoint);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_CX_ENDPOINT,
			       &discovery_cb,
			       &cx_endpoint_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s (%d)", log_strdup(addr),
			conn_err);

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err) {
				LOG_ERR("Scanning failed to start (err %d)",
					err);
			}
		}

		return;
	}

	LOG_INF("Connected: %s", log_strdup(addr));

	gatt_discover(conn);

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr),
		reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}

	gatt_discover(conn);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s, rssi: %d, connectable: %d",
		log_strdup(addr), device_info->recv_info->rssi, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

static int cx_endpoint_client_init(void)
{
	int err;
	struct bt_cx_endpoint_client_init_param init = {
		.cb = {
			.received = ble_data_received,
		}
	};

	err = bt_cx_endpoint_client_init(&cx_endpoint_client, &init);
	if (err) {
		LOG_ERR("CX_ENDPOINT Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("CX_ENDPOINT Client module initialized");
	return err;
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static int scan_init(void)
{
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, DEVICE_NAME_FILTER);
	if (err) {
		LOG_ERR("Scanning filter %d cannot be set (err %d)",BT_SCAN_FILTER_TYPE_NAME , err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}

void app_start_scanning(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_WRN("Scanning failed to start (err %d)", err);
		return;
	}
	LOG_INF("Scanning successfully started");
}

void app_stop_scanning(void)
{
	int err;

	err = bt_scan_stop();
	if (err) {
		LOG_WRN("Scanning failed to stop (err %d)", err);
		return;
	}
	LOG_INF("Scanning successfully stopped");	

}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	int err;

	LOG_INF("Button pressed!");

	if( 
		(has_changed & BUTTON_SCAN) && 
		(button_state & BUTTON_SCAN)
	){
		app_start_scanning();
	}
	else if(
		(has_changed & BUTTON_SCAN) && 
		!(button_state & BUTTON_SCAN)
	){
		app_stop_scanning();
	}

	if( (has_changed & BUTTON_SEND_DATA) ){
		err = bt_cx_endpoint_client_send(&cx_endpoint_client,(const uint8_t *)&button_state,1);
		LOG_INF("Send data result: %d",err);
		dk_set_led(BTN_STATUS_LED,button_state);
	}

}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_INF("Cannot init buttons (err: %d)", err);
	}

	return err;
}

void main(void)
{
	int err;
	int blink_status = 0;

	err = dk_leds_init();
	if (err) {
		LOG_INF("LEDs init failed (err %d)", err);
		return;
	}

	err = init_button();
	if (err) {
		LOG_INF("Buttons init failed (err %d)", err);
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_cb_register(&conn_callbacks);

	err = scan_init();
	if(err){
		LOG_ERR("Scan Init failed: %d",err);
		return;
	}

	err = cx_endpoint_client_init();
	if(err){
		LOG_ERR("CX_ENDPOINT Client init failed: %d",err);
		return;
	}

	printk("Starting Bluetooth Central CX Endpoint Client example\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
