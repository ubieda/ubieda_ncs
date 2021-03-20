/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_CX_ENDPOINT_CLIENT_H_
#define BT_CX_ENDPOINT_CLIENT_H_

/**
 * @file
 * @defgroup bt_cx_endpoint_client Bluetooth LE GATT CX_ENDPOINT Client API
 * @{
 * @brief API for the Bluetooth LE GATT Nordic UART Service (CX_ENDPOINT) Client.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

/** @brief Handles on the connected peer device that are needed to interact with
 * the device.
 */
struct bt_cx_endpoint_client_handles {

        /** Handle of the CX_ENDPOINT RX characteristic, as provided by
	 *  a discovery.
         */
	uint16_t rx;

        /** Handle of the CX_ENDPOINT TX characteristic, as provided by
	 *  a discovery.
         */
	uint16_t tx;

        /** Handle of the CCC descriptor of the CX_ENDPOINT TX characteristic,
	 *  as provided by a discovery.
         */
	uint16_t tx_ccc;
};

/** @brief CX_ENDPOINT Client callback structure. */
struct bt_cx_endpoint_client_cb {
	/** @brief Data received callback.
	 *
	 * The data has been received as a notification of the CX_ENDPOINT TX
	 * Characteristic.
	 *
	 * @param[in] data Received data.
	 * @param[in] len Length of received data.
	 *
	 * @retval BT_GATT_ITER_CONTINUE To keep notifications enabled.
	 * @retval BT_GATT_ITER_STOP To disable notifications.
	 */
	uint8_t (*received)(const uint8_t *data, uint16_t len);

	/** @brief Data sent callback.
	 *
	 * The data has been sent and written to the CX_ENDPOINT RX Characteristic.
	 *
	 * @param[in] err ATT error code.
	 * @param[in] data Transmitted data.
	 * @param[in] len Length of transmitted data.
	 */
	void (*sent)(uint8_t err, const uint8_t *data, uint16_t len);

	/** @brief TX notifications disabled callback.
	 *
	 * TX notifications have been disabled.
	 */
	void (*unsubscribed)(void);
};

/** @brief CX_ENDPOINT Client structure. */
struct bt_cx_endpoint_client {

        /** Connection object. */
	struct bt_conn *conn;

        /** Internal state. */
	atomic_t state;

        /** Handles on the connected peer device that are needed
         * to interact with the device.
         */
	struct bt_cx_endpoint_client_handles handles;

        /** GATT subscribe parameters for CX_ENDPOINT TX Characteristic. */
	struct bt_gatt_subscribe_params tx_notif_params;

        /** GATT write parameters for CX_ENDPOINT RX Characteristic. */
	struct bt_gatt_write_params rx_write_params;

        /** Application callbacks. */
	struct bt_cx_endpoint_client_cb cb;
};

/** @brief CX_ENDPOINT Client initialization structure. */
struct bt_cx_endpoint_client_init_param {

        /** Callbacks provided by the user. */
	struct bt_cx_endpoint_client_cb cb;
};

int bt_cx_endpoint_client_init(struct bt_cx_endpoint_client *cx_endpoint,
		       const struct bt_cx_endpoint_client_init_param *init_param);
int bt_cx_endpoint_client_send(struct bt_cx_endpoint_client *cx_endpoint, const uint8_t *data,
		       uint16_t len);
int bt_cx_endpoint_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_cx_endpoint_client *cx_endpoint);
int bt_cx_endpoint_subscribe_receive(struct bt_cx_endpoint_client *cx_endpoint);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_CX_ENDPOINT_CLIENT_H_ */
