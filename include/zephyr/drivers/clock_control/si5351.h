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

typedef enum
{
    si5351_xtal_load_6pf = 1,
    si5351_xtal_load_8pf,
    si5351_xtal_load_10pf,
} si5351_xtal_load_t;

typedef enum
{
    si5351_clkin_div_1,
    si5351_clkin_div_2,
    si5351_clkin_div_4,
    si5351_clkin_div_8,
} si5351_clkin_div_t;

typedef struct
{
    si5351_pll_clock_source_t clock_source;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
} si5351_pll_parameters_t;

typedef struct
{
    si5351_clkin_div_t clkin_div;
    si5351_xtal_load_t xtal_load;
    si5351_pll_parameters_t plla;
    si5351_pll_parameters_t pllb;
} si5351_parameters_t;

typedef enum
{
    si5351_output_output_enabled,
    si5351_output_output_disabled,
} si5351_output_output_t;

typedef enum
{
    si5351_output_powered_up,
    si5351_output_powered_down,
} si5351_output_powered_t;

typedef enum
{
    si5351_output_integer_mode_disabled,
    si5351_output_integer_mode_enabled,
} si5351_output_integer_mode_t;

typedef enum
{
    si5351_output_multisynth_source_plla,
    si5351_output_multisynth_source_pllb,
} si5351_output_multisynth_source_t;

typedef enum
{
    si5351_output_invert_disabled,
    si5351_output_invert_enabled,
} si5351_output_invert_t;

typedef enum
{
    si5351_output_clk_source_xtal,
    si5351_output_clk_source_clkin,
    si5351_output_clk_source_multisynth = 3,
} si5351_output_clk_source_t;

typedef enum
{
    si5351_output_drive_strength_2ma,
    si5351_output_drive_strength_4ma,
    si5351_output_drive_strength_6ma,
    si5351_output_drive_strength_8ma,
} si5351_output_drive_strength_t;

typedef enum
{
    si5351_output_r_1,
    si5351_output_r_2,
    si5351_output_r_4,
    si5351_output_r_8,
    si5351_output_r_16,
    si5351_output_r_32,
    si5351_output_r_64,
    si5351_output_r_128,
} si5351_output_r_t;

typedef struct
{
    si5351_output_output_t output_enabled;
    si5351_output_powered_t powered_up;
    si5351_output_integer_mode_t integer_mode;
    si5351_output_multisynth_source_t multisynth_source;
    si5351_output_invert_t invert;
    si5351_output_clk_source_t clock_source;
    si5351_output_drive_strength_t drive_strength;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
    si5351_output_r_t r : 3;
    bool divide_by_four;
    uint8_t phase_offset : 7;
} si5351_output_parameters_t;

typedef struct
{
    bool sys_init;
    bool plla_loss_of_lock;
    bool pllb_loss_of_lock;
    bool clkin_loss_of_signal;
    bool xtal_loss_of_signal;
    uint8_t revision_id : 2;
} si5351_status_t;

typedef enum
{
    si5351_pll_mask_a = 1 << 0,
    si5351_pll_mask_b = 1 << 1,
} si5351_pll_mask_t;

int si5351_reset_pll(const struct device *dev, si5351_pll_mask_t pll);

int si5351_output_get_parameters(const struct device *dev, si5351_output_parameters_t *parameters);
int si5351_output_set_parameters(const struct device *dev, si5351_output_parameters_t const *parameters);

int si5351_tune_pll(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_parameters_t const *parameters);
int si5351_set_output(const struct device *dev, uint8_t output_index, si5351_output_output_t state);

int si5351_get_status(const struct device *dev, si5351_status_t *status);

#endif // ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_