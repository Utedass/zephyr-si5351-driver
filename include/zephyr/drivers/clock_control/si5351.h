/*
 * Copyright (c) 2025 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_

#include <zephyr/device.h>

typedef enum
{
    si5351_pll_clock_source_xtal,
    si5351_pll_clock_source_clkin,
} si5351_pll_clock_source_t;

typedef struct
{
    si5351_pll_clock_source_t clock_source;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
} si5351_pll_parameters_t;

typedef enum
{
    si5351_clk_powered_up,
    si5351_clk_powered_down,
} si5351_clk_powered_t;

typedef enum
{
    si5351_clk_integer_mode_disabled,
    si5351_clk_integer_mode_enabled,
} si5351_clk_integer_mode_t;

typedef enum
{
    si5351_clk_multisynth_source_plla,
    si5351_clk_multisynth_source_pllb,
} si5351_clk_multisynth_source_t;

typedef enum
{
    si5351_clk_invert_disabled,
    si5351_clk_invert_enabled,
} si5351_clk_invert_t;

typedef enum
{
    si5351_clk_source_xtal,
    si5351_clk_source_clkin,
    si5351_clk_source_multisynth,
} si5351_clk_source_t;

typedef enum
{
    si5351_clk_drive_strength_2ma,
    si5351_clk_drive_strength_4ma,
    si5351_clk_drive_strength_6ma,
    si5351_clk_drive_strength_8ma,
} si5351_clk_drive_strength_t;

typedef struct
{
    si5351_clk_powered_t powered;
    si5351_clk_integer_mode_t integer_mode;
    si5351_clk_multisynth_source_t multisynth_source;
    si5351_clk_invert_t invert;
    si5351_clk_source_t source;
    si5351_clk_drive_strength_t drive_strength;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
    uint8_t r0 : 3;
    uint8_t d4 : 2;
} si5351_clk_parameters_t;

int si5351_dummy(const struct device *dev);

#endif // ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_