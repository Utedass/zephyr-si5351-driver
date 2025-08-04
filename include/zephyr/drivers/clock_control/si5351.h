/*
 * Copyright (c) 2025 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_

#include <zephyr/device.h>

#ifdef CONFIG_CLOCK_CONTROL_SI5351_OVERRIDE_VCO_MIN_FREQUENCY
#define SI5351_VCO_MIN_FREQUENCY CONFIG_CLOCK_CONTROL_SI5351_VCO_MIN_FREQUENCY
#else
#define SI5351_VCO_MIN_FREQUENCY 600U // 600 MHz
#endif

#ifdef CONFIG_CLOCK_CONTROL_SI5351_OVERRIDE_VCO_MAX_FREQUENCY
#define SI5351_VCO_MAX_FREQUENCY CONFIG_CLOCK_CONTROL_SI5351_VCO_MAX_FREQUENCY
#else
#define SI5351_VCO_MAX_FREQUENCY 900U // 900 MHz
#endif

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
    si5351_pll_index_a = 0,
    si5351_pll_index_b = 1,
} si5351_pll_index_t;

typedef enum
{
    si5351_pll_mask_a = 1 << si5351_pll_index_a,
    si5351_pll_mask_b = 1 << si5351_pll_index_b,
} si5351_pll_mask_t;

typedef enum
{
    si5351_pll_source_xtal,
    si5351_pll_source_clkin,
} si5351_pll_source_t;

typedef enum
{
    si5351_output_index_0 = 0,
    si5351_output_index_1 = 1,
    si5351_output_index_2 = 2,
    si5351_output_index_3 = 3,
    si5351_output_index_4 = 4,
    si5351_output_index_5 = 5,
    si5351_output_index_6 = 6,
    si5351_output_index_7 = 7,
} si5351_output_index_t;

typedef enum
{
    si5351_output_mask_0 = 1 << si5351_output_index_0,
    si5351_output_mask_1 = 1 << si5351_output_index_1,
    si5351_output_mask_2 = 1 << si5351_output_index_2,
    si5351_output_mask_3 = 1 << si5351_output_index_3,
    si5351_output_mask_4 = 1 << si5351_output_index_4,
    si5351_output_mask_5 = 1 << si5351_output_index_5,
    si5351_output_mask_6 = 1 << si5351_output_index_6,
    si5351_output_mask_7 = 1 << si5351_output_index_7,
} si5351_output_mask_t;

typedef enum
{
    si5351_output_source_xtal,
    si5351_output_source_clkin,
    si5351_output_source_multisynth,
} si5351_output_source_t;

#ifdef CONFIG_CLOCK_CONTROL_SI5351_DOUBLE_PRECISION
typedef double si5351_frequency_t;
typedef double si5351_ratio_t;
#else
typedef float si5351_frequency_t;
typedef float si5351_ratio_t;
#endif

int si5351_reapply_configuration(const struct device *dev);
int si5351_apply_dt_settings(const struct device *dev);

int si5351_set_clkin(const struct device *dev, uint32_t frequency, si5351_clkin_div_t div);

int si5351_is_xtal_running(const struct device *dev);

int si5351_pll_soft_reset(const struct device *dev, si5351_pll_mask_t pll_mask);
int si5351_pll_set_frequency(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_source_t source, si5351_frequency_t frequency);
int si5351_pll_set_multiplier(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_ratio_t multiplier);
int si5351_pll_set_multiplier_abc(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t a, uint32_t b, uint32_t c);
int si5351_pll_set_multiplier_parameters(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t p1, uint32_t p2, uint32_t p3);
int si5351_pll_set_multiplier_integer(const struct device *dev, si5351_pll_mask_t pll_mask, uint8_t multiplier);
int si5351_pll_set_divider_fixed(const struct device *dev, si5351_pll_mask_t pll_mask, bool is_fixed);

int si5351_pll_get_frequency(const struct device *dev, si5351_pll_index_t pll_index, si5351_frequency_t *frequency);
int si5351_pll_get_multiplier(const struct device *dev, si5351_pll_index_t pll_index, si5351_ratio_t *multiplier);

int si5351_pll_is_fixed(const struct device *dev, si5351_pll_index_t pll_index);
int si5351_pll_is_locked(const struct device *dev, si5351_pll_index_t pll_index);

int si5351_output_set_source(const struct device *dev, si5351_output_mask_t output_mask, si5351_output_source_t source);
int si5351_output_set_multisynth_source(const struct device *dev, si5351_output_mask_t output_mask, si5351_output_multisynth_source_t source);
int si5351_output_set_frequency(const struct device *dev, si5351_output_mask_t output_mask, si5351_frequency_t frequency);
int si5351_output_set_divider(const struct device *dev, si5351_output_mask_t output_mask, si5351_ratio_t divider);
int si5351_output_set_divider_abc(const struct device *dev, si5351_output_mask_t output_mask, uint32_t a, uint32_t b, uint32_t c);
int si5351_output_set_divider_parameters(const struct device *dev, si5351_output_mask_t output_mask, uint32_t p1, uint32_t p2, uint32_t p3);
int si5351_output_set_divider_integer(const struct device *dev, si5351_output_mask_t output_mask, uint8_t integer);
int si5351_output_set_divider_fixed(const struct device *dev, si5351_output_mask_t output_mask, bool is_fixed);
int si5351_output_set_powered_down(const struct device *dev, si5351_output_mask_t output_mask, bool is_powered_down);
int si5351_output_set_output_enabled(const struct device *dev, si5351_output_mask_t output_mask, bool is_enabled);
int si5351_output_set_output_enable_mask(const struct device *dev, si5351_output_mask_t output_mask, bool is_masked);
int si5351_output_set_inverted(const struct device *dev, si5351_output_mask_t output_mask, bool is_inverted);
int si5351_output_set_phase_offset_ps(const struct device *dev, si5351_output_mask_t output_mask, uint32_t pico_seconds);
int si5351_output_set_phase_offset_val(const struct device *dev, si5351_output_mask_t output_mask, uint8_t val);

int si5351_output_get_frequency(const struct device *dev, si5351_output_index_t output_index, si5351_frequency_t *frequency);
int si5351_output_get_divider(const struct device *dev, si5351_output_index_t output_index, si5351_ratio_t *divider);

#endif // ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SI5351_H_