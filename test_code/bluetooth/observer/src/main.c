/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic Battery Service Client sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <inttypes.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/bas_client.h>
#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

#define KEY_READVAL_MASK        DK_BTN1_MSK
#define KEY_READVAL2_MASK       DK_BTN2_MSK

#define RUN_STATUS_LED          DK_LED1
#define RUN_LED_BLINK_INTERVAL  100

#define SCANNING_STATUS_LED     DK_LED2

#define DEVICE_NAME_FILTER      CONFIG_BT_DEVICE_NAME

typedef struct {
	uint16_t mfgr_id;
	uint32_t button_presses;
} app_payload_t;

static app_payload_t app_payload = {
	.mfgr_id = CONFIG_BT_COMPANY_ID,
	.button_presses = 0,
};

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s, rssi: %d, connectable: %s\n",
		addr, device_info->recv_info->rssi ,connectable ? "yes" : "no");
	LOG_HEXDUMP_INF(device_info->adv_data->data, device_info->adv_data->len, "Adv-data");
}

static void scan_filter_no_match(struct bt_scan_device_info *device_info,
				 bool connectable)
{
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match, NULL, NULL);

static void scan_init(void)
{
	int err;

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 0,
		.scan_param = NULL,
		.conn_param = NULL,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, DEVICE_NAME_FILTER);
	if (err) {
		printk("Scanning filter %d cannot be set (err %d)\n",BT_SCAN_FILTER_TYPE_NAME , err);

		return;
	}

	struct bt_scan_manufacturer_data mfgr_filter;
	mfgr_filter.data = (uint8_t *)&app_payload,
	mfgr_filter.data_len = sizeof(app_payload.mfgr_id),

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA,&mfgr_filter);
	if (err) {
		printk("Scanning filter %d cannot be set (err %d)\n",BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA , err);

		return;
	}
}

void app_start_scanning(uint8_t scan_filter)
{
	int err;

	err = bt_scan_filter_enable(scan_filter, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);

		return;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}
	printk("Scanning successfully started\n");
}

void app_stop_scanning(void)
{
	int err;

	bt_scan_filter_disable();

	err = bt_scan_stop();
	if (err) {
		printk("Scanning failed to stop (err %d)\n", err);
		return;
	}
	printk("Scanning successfully stopped\n");	

}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & KEY_READVAL_MASK || has_changed & KEY_READVAL2_MASK) {
		printk("Button was pressed - State: %d\n",button_state);
		if(button_state & KEY_READVAL_MASK){
			dk_set_led(SCANNING_STATUS_LED, 1);
			app_start_scanning(BT_SCAN_NAME_FILTER);
		}
		else if(button_state & KEY_READVAL2_MASK){
			dk_set_led(SCANNING_STATUS_LED, 1);
			app_start_scanning(BT_SCAN_MANUFACTURER_DATA_FILTER);
		}
		else{
			dk_set_led(SCANNING_STATUS_LED, 0);
			app_stop_scanning();
		}
	}
}

void main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Bluetooth Observer Test-Code\n");
	LOG_INF("Testing log");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	scan_init();

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return;
	}

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}	
}
