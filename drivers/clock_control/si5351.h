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

typedef struct
{
} si5351_data_t;

typedef struct
{
    struct i2c_dt_spec i2c;
} si5351_config_t;

typedef struct
{
} si5351_output_data_t;

typedef struct
{
    const struct device *parent; // Back-reference to the parent Si5351 device
    uint8_t output_index;        // 0–2 for Si5351A
} si5351_output_config_t;

#endif // ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
