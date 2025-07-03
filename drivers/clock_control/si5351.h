/*
 * Copyright (c) 2025 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

struct si5351_config
{
    struct i2c_dt_spec i2c;
};

#endif
