#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Point to Ubieda's NCS root directory.
set(UBIEDA_NCS_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "Ubieda's NCS root directory")

zephyr_include_directories(include)

add_subdirectory(subsys)
