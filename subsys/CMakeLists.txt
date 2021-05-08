#
# Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_BT)
  add_subdirectory(bluetooth)
endif()

if (CONFIG_BASIC_MODULE)
  add_subdirectory(basic_module)
endif()
