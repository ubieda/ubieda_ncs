#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_BT_CX_ENDPOINT cx_endpoint.c)
zephyr_sources_ifdef(CONFIG_BT_CX_ENDPOINT_CLIENT cx_endpoint_client.c)
