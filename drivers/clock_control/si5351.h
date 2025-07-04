/*
 * Copyright (c) 2025 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>

#define SI5351_INIT_PRIORITY CONFIG_CLOCK_CONTROL_SI5351_INIT_PRIORITY

#define SI5351_REG_ADR_STATUS 0x0

struct si5351_data
{
};

struct si5351_config
{
    struct i2c_dt_spec i2c;
};

#endif // ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
