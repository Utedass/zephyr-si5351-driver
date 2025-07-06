/*
 * Copyright (c) 2025 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control/si5351.h>

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
    si5351_output_parameters_t current_parameters;
} si5351_output_data_t;

typedef struct
{
    uint8_t powered;
    bool integer_mode;
    uint8_t multisynth_source;
    bool invert;
    uint8_t clock_source;
    uint8_t drive_strength;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
    uint8_t r0;
    bool divide_by_four;
    uint8_t phase_offset : 7;
} si5351_output_default_config_t;

typedef struct
{
    const struct device *parent; // Back-reference to the parent Si5351 device
    uint8_t output_index;        // 0–2 for Si5351A
    si5351_output_default_config_t default_config;
} si5351_output_config_t;

#endif // ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
