/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_CX_ENDPOINT_H_
#define BT_CX_ENDPOINT_H_

/**@file
 * @defgroup bt_cx_endpoint Service API
 * @{
 * @brief API for the Generic Croxel Endpoint Service.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/** @brief CX_ENDPOINT Service UUID. */
#define BT_UUID_CX_ENDPOINT_VAL \
	BT_UUID_128_ENCODE(0x0a000001, 0xcafe, 0xcafe, 0xcafe, 0xdeadbeefcafe)

/** @brief Send Endpoint Characteristic UUID. */
#define BT_UUID_CX_ENDPOINT_SEND_VAL \
	BT_UUID_128_ENCODE(0x0a000002, 0xcafe, 0xcafe, 0xcafe, 0xdeadbeefcafe)

/** @brief Receive Endpoint Characteristic UUID. */
#define BT_UUID_CX_ENDPOINT_RECV_VAL \
	BT_UUID_128_ENCODE(0x0a000003, 0xcafe, 0xcafe, 0xcafe, 0xdeadbeefcafe)


#define BT_UUID_CX_ENDPOINT           BT_UUID_DECLARE_128(BT_UUID_CX_ENDPOINT_VAL)
#define BT_UUID_CX_ENDPOINT_SEND    BT_UUID_DECLARE_128(BT_UUID_CX_ENDPOINT_SEND_VAL)
#define BT_UUID_CX_ENDPOINT_RECV       BT_UUID_DECLARE_128(BT_UUID_CX_ENDPOINT_RECV_VAL)

/** @brief Callback struct used by the CX_ENDPOINT Service. */
struct bt_cx_endpoint_cb {
	/** Received data callback. */
	void (*recv_cb)(const uint8_t *data, uint16_t len);
};

int bt_cx_endpoint_init(struct bt_cx_endpoint_cb *callbacks);
int bt_cx_endpoint_send_data(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_CX_ENDPOINT_H_ */
