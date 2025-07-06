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

#define SI5351_REG_STATUS_ADR 0x00
#define SI5351_REG_OEB_ADR 0x03
#define SI5351_REG_OEB_MASK_ADR 0x09
#define SI5351_REG_PLL_CFG_ADR 0x0f

#define SI5351_REG_CLK_OUT_CTRL_ADR_BASE 0x10
#define SI5351_REG_CLK_OUT_CTRL_SIZE 0x01

#define SI5351_REG_PLL_X_ADR_BASE 0x1a
#define SI5351_REG_PLL_X_SIZE 0x08
#define SI5351_REG_PLL_X_P3M_OFFSET 0x00
#define SI5351_REG_PLL_X_P3L_OFFSET 0x01
#define SI5351_REG_PLL_X_P1H_OFFSET 0x02
#define SI5351_REG_PLL_X_P1M_OFFSET 0x03
#define SI5351_REG_PLL_X_P1L_OFFSET 0x04
#define SI5351_REG_PLL_X_P3HP2H_OFFSET 0x05
#define SI5351_REG_PLL_X_P2M_OFFSET 0x06
#define SI5351_REG_PLL_X_P2L_OFFSET 0x07

#define SI5351_REG_CLK_OUT_X_ADR_BASE 0x2a
#define SI5351_REG_CLK_OUT_X_SIZE 0x08
#define SI5351_REG_CLK_OUT_X_P3M_OFFSET 0x00
#define SI5351_REG_CLK_OUT_X_P3L_OFFSET 0x01
#define SI5351_REG_CLK_OUT_X_P1H_OFFSET 0x02
#define SI5351_REG_CLK_OUT_X_P1M_OFFSET 0x03
#define SI5351_REG_CLK_OUT_X_P1L_OFFSET 0x04
#define SI5351_REG_CLK_OUT_X_P3HP2H_OFFSET 0x05
#define SI5351_REG_CLK_OUT_X_P2M_OFFSET 0x06
#define SI5351_REG_CLK_OUT_X_P2L_OFFSET 0x07

#define SI5351_REG_CLK_OUT_PHASE_OFFSET_X_ADR_BASE 0xa5
#define SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE 0x01

#define SI5351_REG_PLL_RESET_ADR 0x75
#define SI5351_REG_XTAL_LOAD_ADR 0xb7
#define SI5351_REG_FANOUT_ADR 0xbb

typedef struct
{
    uint8_t clkin_div;
    uint8_t xtal_load;
    si5351_pll_parameters_t plla;
    si5351_pll_parameters_t pllb;
} si5351_default_config_t;

typedef struct
{
    si5351_parameters_t current_parameters;
} si5351_data_t;

typedef struct
{
    struct i2c_dt_spec i2c;
    si5351_default_config_t default_config;
} si5351_config_t;

typedef struct
{
    si5351_output_parameters_t current_parameters;
} si5351_output_data_t;

typedef struct
{
    bool output_enabled;
    bool powered_up;
    bool integer_mode;
    uint8_t multisynth_source;
    bool invert;
    uint8_t clock_source;
    uint8_t drive_strength;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
    uint8_t r;
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
