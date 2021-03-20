/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Generic Croxel Endpoint Service
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/cx_endpoint.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_cx_endpoint, CONFIG_BT_CX_ENDPOINT_LOG_LEVEL);

static bool                   		notify_enabled;
static struct bt_cx_endpoint_cb 	cx_endpoint_cb;

static void cx_endpointlc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t received_msg(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, conn);

	if (cx_endpoint_cb.recv_cb) {
		cx_endpoint_cb.recv_cb((const uint8_t *)buf,len);
	}

	return len;
}

/* Service Declaration */
BT_GATT_SERVICE_DEFINE(cx_endpoint_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_CX_ENDPOINT),
	BT_GATT_CHARACTERISTIC(BT_UUID_CX_ENDPOINT_SEND,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(cx_endpointlc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_CX_ENDPOINT_RECV,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, received_msg, NULL),
);

int bt_cx_endpoint_init(struct bt_cx_endpoint_cb *callbacks)
{
	if (callbacks) {
		cx_endpoint_cb.recv_cb    = callbacks->recv_cb;
	}

	return 0;
}

int bt_cx_endpoint_send_data(const uint8_t *data, uint16_t len)
{
	if (!notify_enabled) {
		return -EACCES;
	}

	if(!data){
		return -EINVAL;
	}

	return bt_gatt_notify(NULL, &cx_endpoint_svc.attrs[2],
			      data,
			      len);
}
