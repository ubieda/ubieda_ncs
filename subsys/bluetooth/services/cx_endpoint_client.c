/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/cx_endpoint.h>
#include <bluetooth/services/cx_endpoint_client.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(cx_endpoint_c, CONFIG_BT_CX_ENDPOINT_CLIENT_LOG_LEVEL);

enum {
	CX_ENDPOINT_C_INITIALIZED,
	CX_ENDPOINT_C_TX_NOTIF_ENABLED,
	CX_ENDPOINT_C_RX_WRITE_PENDING
};

static uint8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	struct bt_cx_endpoint_client *cx_endpoint;

	/* Retrieve CX_ENDPOINT Client module context. */
	cx_endpoint = CONTAINER_OF(params, struct bt_cx_endpoint_client, tx_notif_params);

	if (!data) {
		LOG_DBG("[UNSUBSCRIBED]");
		params->value_handle = 0;
		atomic_clear_bit(&cx_endpoint->state, CX_ENDPOINT_C_TX_NOTIF_ENABLED);
		if (cx_endpoint->cb.unsubscribed) {
			cx_endpoint->cb.unsubscribed();
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[NOTIFICATION] data %p length %u", data, length);
	if (cx_endpoint->cb.received) {
		return cx_endpoint->cb.received(data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void on_sent(struct bt_conn *conn, uint8_t err,
		    struct bt_gatt_write_params *params)
{
	struct bt_cx_endpoint_client *cx_endpoint_c;
	const void *data;
	uint16_t length;

	/* Retrieve CX_ENDPOINT Client module context. */
	cx_endpoint_c = CONTAINER_OF(params, struct bt_cx_endpoint_client, rx_write_params);

	/* Make a copy of volatile data that is required by the callback. */
	data = params->data;
	length = params->length;

	atomic_clear_bit(&cx_endpoint_c->state, CX_ENDPOINT_C_RX_WRITE_PENDING);
	if (cx_endpoint_c->cb.sent) {
		cx_endpoint_c->cb.sent(err, data, length);
	}
}

int bt_cx_endpoint_client_init(struct bt_cx_endpoint_client *cx_endpoint_c,
		       const struct bt_cx_endpoint_client_init_param *cx_endpoint_c_init)
{
	if (!cx_endpoint_c || !cx_endpoint_c_init) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&cx_endpoint_c->state, CX_ENDPOINT_C_INITIALIZED)) {
		return -EALREADY;
	}

	memcpy(&cx_endpoint_c->cb, &cx_endpoint_c_init->cb, sizeof(cx_endpoint_c->cb));

	return 0;
}

int bt_cx_endpoint_client_send(struct bt_cx_endpoint_client *cx_endpoint_c, const uint8_t *data,
		       uint16_t len)
{
	int err;

	if (!cx_endpoint_c->conn) {
		return -ENOTCONN;
	}

	if (atomic_test_and_set_bit(&cx_endpoint_c->state, CX_ENDPOINT_C_RX_WRITE_PENDING)) {
		return -EALREADY;
	}

	cx_endpoint_c->rx_write_params.func = on_sent;
	cx_endpoint_c->rx_write_params.handle = cx_endpoint_c->handles.rx;
	cx_endpoint_c->rx_write_params.offset = 0;
	cx_endpoint_c->rx_write_params.data = data;
	cx_endpoint_c->rx_write_params.length = len;

	err = bt_gatt_write(cx_endpoint_c->conn, &cx_endpoint_c->rx_write_params);
	if (err) {
		atomic_clear_bit(&cx_endpoint_c->state, CX_ENDPOINT_C_RX_WRITE_PENDING);
	}

	return err;
}

int bt_cx_endpoint_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_cx_endpoint_client *cx_endpoint_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_CX_ENDPOINT)) {
		return -ENOTSUP;
	}
	LOG_DBG("Getting handles from CX_ENDPOINT service.");
	memset(&cx_endpoint_c->handles, 0xFF, sizeof(cx_endpoint_c->handles));

	/* CX_ENDPOINT TX Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_CX_ENDPOINT_SEND);
	if (!gatt_chrc) {
		LOG_ERR("Missing CX_ENDPOINT TX characteristic.");
		return -EINVAL;
	}
	/* CX_ENDPOINT TX */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_CX_ENDPOINT_SEND);
	if (!gatt_desc) {
		LOG_ERR("Missing CX_ENDPOINT TX value descriptor in characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for CX_ENDPOINT TX characteristic.");
	cx_endpoint_c->handles.tx = gatt_desc->handle;
	/* CX_ENDPOINT TX CCC */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_ERR("Missing CX_ENDPOINT TX CCC in characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for CCC of CX_ENDPOINT TX characteristic.");
	cx_endpoint_c->handles.tx_ccc = gatt_desc->handle;

	/* CX_ENDPOINT RX Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_CX_ENDPOINT_RECV);
	if (!gatt_chrc) {
		LOG_ERR("Missing CX_ENDPOINT RX characteristic.");
		return -EINVAL;
	}
	/* CX_ENDPOINT RX */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_CX_ENDPOINT_RECV);
	if (!gatt_desc) {
		LOG_ERR("Missing CX_ENDPOINT RX value descriptor in characteristic.");
		return -EINVAL;
	}
	LOG_DBG("Found handle for CX_ENDPOINT RX characteristic.");
	cx_endpoint_c->handles.rx = gatt_desc->handle;

	/* Assign connection instance. */
	cx_endpoint_c->conn = bt_gatt_dm_conn_get(dm);
	return 0;
}

int bt_cx_endpoint_subscribe_receive(struct bt_cx_endpoint_client *cx_endpoint_c)
{
	int err;

	if (atomic_test_and_set_bit(&cx_endpoint_c->state, CX_ENDPOINT_C_TX_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	cx_endpoint_c->tx_notif_params.notify = on_received;
	cx_endpoint_c->tx_notif_params.value = BT_GATT_CCC_NOTIFY;
	cx_endpoint_c->tx_notif_params.value_handle = cx_endpoint_c->handles.tx;
	cx_endpoint_c->tx_notif_params.ccc_handle = cx_endpoint_c->handles.tx_ccc;
	atomic_set_bit(cx_endpoint_c->tx_notif_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(cx_endpoint_c->conn, &cx_endpoint_c->tx_notif_params);
	if (err) {
		LOG_ERR("Subscribe failed (err %d)", err);
		atomic_clear_bit(&cx_endpoint_c->state, CX_ENDPOINT_C_TX_NOTIF_ENABLED);
	} else {
		LOG_DBG("[SUBSCRIBED]");
	}

	return err;
}
