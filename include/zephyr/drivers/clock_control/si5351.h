/*
 * Copyright (c) 2025 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_

#include <zephyr/device.h>

typedef struct
{
    uint8_t output_number;
} si5351_clock_control_subsys_t;

int si5351_dummy(const struct device *dev);

#endif // ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_