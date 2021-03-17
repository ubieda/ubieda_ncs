/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include <settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define RUN_STATUS_LED          DK_LED1
#define RUN_LED_BLINK_INTERVAL  100

#define USER_BUTTON             DK_BTN1_MSK

typedef struct {
	uint16_t mfgr_id;
	uint32_t button_presses;
} app_payload_t;

static app_payload_t app_payload = {
	.mfgr_id = CONFIG_BT_COMPANY_ID,
	.button_presses = 0,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA,&app_payload, sizeof(app_payload)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
	printk("Hit app error handler\n");
	LOG_PANIC();
	while(1);
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		printk("Button was pressed\n");
		app_payload.button_presses++;
	}
}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

static void app_update_advdata(void)
{
	int err;

	err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		__ASSERT(err == 0,"Update BLE data failed: (err %d)\n",err);
	}

	return;
}

static void app_print_bt_addr(void)
{
	bt_addr_le_t dev_addr = {0};
	size_t no_of_identities = 1;

	bt_id_get(&dev_addr, &no_of_identities);

	char bt_addr_str[40] = {0};

	int err = bt_addr_le_to_str(&dev_addr,bt_addr_str,sizeof(bt_addr_str));
	if(err > 0){
		printk("Device Address - Type: 0x%02X, Address: %s\n",dev_addr.type,bt_addr_str);
	}

	return;
}

void main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Bluetooth Broadcaster Test-Code. Device Name: %s\n",DEVICE_NAME);

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	app_print_bt_addr();

	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	// __ASSERT(err == 58,"Testing assertions, err: %d\n",err);

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
		app_update_advdata();
	}
}
