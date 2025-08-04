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
    SI5351_PLL_CLOCK_SOURCE_XTAL,
    SI5351_PLL_CLOCK_SOURCE_CLKIN,
} si5351_pll_clock_source_t;

typedef enum
{
    SI5351_XTAL_LOAD_6PF = 1,
    SI5351_XTAL_LOAD_8PF,
    SI5351_XTAL_LOAD_10PF,
} si5351_xtal_load_t;

typedef enum
{
    SI5351_CLKIN_DIV_1,
    SI5351_CLKIN_DIV_2,
    SI5351_CLKIN_DIV_4,
    SI5351_CLKIN_DIV_8,
} si5351_clkin_div_t;

typedef struct
{
    si5351_pll_clock_source_t clock_source;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
} si5351_pll_parameters_t;

typedef enum
{
    SI5351_OUTPUT_OUTPUT_ENABLED,
    SI5351_OUTPUT_OUTPUT_DISABLED,
} si5351_output_output_t;

typedef enum
{
    SI5351_OUTPUT_POWERED_UP,
    SI5351_OUTPUT_POWERED_DOWN,
} si5351_output_powered_t;

typedef enum
{
    SI5351_OUTPUT_INTEGER_MODE_DISABLED,
    SI5351_OUTPUT_INTEGER_MODE_ENABLED,
} si5351_output_integer_mode_t;

typedef enum
{
    SI5351_OUTPUT_MULTISYNTH_SOURCE_PLLA,
    SI5351_OUTPUT_MULTISYNTH_SOURCE_PLLB,
} si5351_output_multisynth_source_t;

typedef enum
{
    SI5351_OUTPUT_INVERT_DISABLED,
    SI5351_OUTPUT_INVERT_ENABLED,
} si5351_output_invert_t;

typedef enum
{
    SI5351_OUTPUT_CLK_SOURCE_XTAL,
    SI5351_OUTPUT_CLK_SOURCE_CLKIN,
    SI5351_OUTPUT_CLK_SOURCE_MULTISYNTH = 3,
} si5351_output_clk_source_t;

typedef enum
{
    SI5351_OUTPUT_DRIVE_STRENGTH_2MA,
    SI5351_OUTPUT_DRIVE_STRENGTH_4MA,
    SI5351_OUTPUT_DRIVE_STRENGTH_6MA,
    SI5351_OUTPUT_DRIVE_STRENGTH_8MA,
} si5351_output_drive_strength_t;

typedef enum
{
    SI5351_OUTPUT_R_1,
    SI5351_OUTPUT_R_2,
    SI5351_OUTPUT_R_4,
    SI5351_OUTPUT_R_8,
    SI5351_OUTPUT_R_16,
    SI5351_OUTPUT_R_32,
    SI5351_OUTPUT_R_64,
    SI5351_OUTPUT_R_128,
} si5351_output_r_t;

typedef struct
{
    bool sys_init;
    bool plla_loss_of_lock;
    bool pllb_loss_of_lock;
    bool clkin_loss_of_signal;
    bool xtal_loss_of_signal;
    uint8_t revision_id : 2;
} si5351_status_t;

typedef struct
{
    uint8_t number : 2;
} si5351_revision_number_t;

typedef enum
{
    SI5351_PLL_INDEX_A,
    SI5351_PLL_INDEX_B,
} si5351_pll_index_t;

typedef enum
{
    SI5351_PLL_MASK_A = 1 << SI5351_PLL_INDEX_A,
    SI5351_PLL_MASK_B = 1 << SI5351_PLL_INDEX_B,
} si5351_pll_mask_t;

typedef enum
{
    SI5351_PLL_SOURCE_XTAL,
    SI5351_PLL_SOURCE_CLKIN,
} si5351_pll_source_t;

typedef enum
{
    SI5351_OUTPUT_INDEX_0,
    SI5351_OUTPUT_INDEX_1,
    SI5351_OUTPUT_INDEX_2,
    SI5351_OUTPUT_INDEX_3,
    SI5351_OUTPUT_INDEX_4,
    SI5351_OUTPUT_INDEX_5,
    SI5351_OUTPUT_INDEX_6,
    SI5351_OUTPUT_INDEX_7,
} si5351_output_index_t;

typedef enum
{
    SI5351_OUTPUT_MASK_0 = 1 << SI5351_OUTPUT_INDEX_0,
    SI5351_OUTPUT_MASK_1 = 1 << SI5351_OUTPUT_INDEX_1,
    SI5351_OUTPUT_MASK_2 = 1 << SI5351_OUTPUT_INDEX_2,
    SI5351_OUTPUT_MASK_3 = 1 << SI5351_OUTPUT_INDEX_3,
    SI5351_OUTPUT_MASK_4 = 1 << SI5351_OUTPUT_INDEX_4,
    SI5351_OUTPUT_MASK_5 = 1 << SI5351_OUTPUT_INDEX_5,
    SI5351_OUTPUT_MASK_6 = 1 << SI5351_OUTPUT_INDEX_6,
    SI5351_OUTPUT_MASK_7 = 1 << SI5351_OUTPUT_INDEX_7,
} si5351_output_mask_t;

typedef enum
{
    SI5351_OUTPUT_SOURCE_XTAL,
    SI5351_OUTPUT_SOURCE_CLKIN,
    SI5351_OUTPUT_SOURCE_MULTISYNTH,
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

int si5351_get_revision_number(const struct device *dev, si5351_revision_number_t *revision_number);

bool si5351_is_xtal_running(const struct device *dev);
bool si5351_is_clkin_running(const struct device *dev);

int si5351_pll_soft_reset(const struct device *dev, si5351_pll_mask_t pll_mask);
int si5351_pll_set_frequency(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_source_t source, si5351_frequency_t frequency);
int si5351_pll_set_multiplier(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_ratio_t multiplier);
int si5351_pll_set_multiplier_abc(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t a, uint32_t b, uint32_t c);
int si5351_pll_set_multiplier_parameters(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_parameters_t parameters);
int si5351_pll_set_multiplier_integer(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t multiplier);
int si5351_pll_set_divider_fixed(const struct device *dev, si5351_pll_mask_t pll_mask, bool is_fixed);

int si5351_pll_get_frequency(const struct device *dev, si5351_pll_index_t pll_index, si5351_frequency_t *frequency);
int si5351_pll_get_multiplier(const struct device *dev, si5351_pll_index_t pll_index, si5351_ratio_t *multiplier);

bool si5351_pll_is_fixed(const struct device *dev, si5351_pll_index_t pll_index);
bool si5351_pll_is_locked(const struct device *dev, si5351_pll_index_t pll_index);

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