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
#define SI5351_REG_INTERRUPT_ADR 0x01
#define SI5351_REG_INTERRUPT_MASK_ADR 0x02
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

// Enums must be in same order as in bindings file skyworks,si5351.yaml

// ======== SI5351 PARENT DT CONFIG START ========
typedef enum
{
    SI5351_MODEL_SI5351A_B_GT,
    SI5351_MODEL_SI5351A_B_GM1,
    SI5351_MODEL_SI5351A_B_GM,
    SI5351_MODEL_SI5351B_B_GM1,
    SI5351_MODEL_SI5351B_B_GM,
    SI5351_MODEL_SI5351C_B_GM1,
    SI5351_MODEL_SI5351C_B_GM
} si5351_model_t;

typedef enum
{
    SI5351_PLL_CLOCK_SOURCE_DT_CONFIG_XTAL,
    SI5351_PLL_CLOCK_SOURCE_DT_CONFIG_CLKIN,
} si5351_pll_clock_source_dt_config_t;

typedef struct
{
    si5351_pll_clock_source_dt_config_t clock_source;
    bool fixed_multiplier;

    bool using_frequency;
    uint32_t frequency;
    uint32_t frequency_fractional;

    bool using_multiplier;
    uint32_t a;
    uint32_t b;
    uint32_t c;

    bool using_multiplier_parameters;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;

    bool using_integer_mode;
    bool integer_mode;
} si5351_pll_dt_config_t;

typedef struct
{
    si5351_model_t model;

    uint8_t xtal_frequency;
    uint8_t xtal_load;

    uint32_t clkin_frequency;
    uint8_t clkin_div;

    si5351_pll_dt_config_t plla;
    si5351_pll_dt_config_t pllb;
} si5351_dt_config_t;

typedef struct
{
    struct i2c_dt_spec i2c;
    si5351_dt_config_t dt_config;
    uint8_t num_okay_clocks;
} si5351_config_t;

// ======== SI5351 PARENT DT CONFIG END ========

// ======== SI5351 CHILD DT CONFIG START ========

typedef struct
{
    uint8_t output_index;
    bool output_enabled;
    bool powered_up;
    bool integer_mode;
    uint8_t clock_source;
    uint8_t multisynth_source;
    uint8_t drive_strength;
    bool invert;
    bool fixed_divider;

    bool using_frequency;
    uint32_t frequency;
    uint32_t frequency_fractional;

    bool using_multiplier;
    uint32_t a;
    uint32_t b;
    uint32_t c;

    bool using_multiplier_parameters;
    uint32_t p1 : 18;
    uint32_t p2 : 20;
    uint32_t p3 : 20;
    uint8_t r;
    bool divide_by_four;

    bool using_phase_offset;
    uint8_t phase_offset : 7;

    bool using_phase_offset_ps;
    uint32_t phase_offset_ps;
} si5351_output_dt_config_t;

typedef struct
{
    const struct device *parent; // Back-reference to the parent Si5351 device
    si5351_output_dt_config_t dt_config;
} si5351_output_config_t;

// ======== SI5351 PARENT DT CONFIG END ========

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
    bool output_present;
    si5351_output_parameters_t *current_parameters;
} si5351_children_t;

typedef struct
{
    si5351_clkin_div_t clkin_div;
    si5351_xtal_load_t xtal_load;
    si5351_pll_parameters_t plla;
    si5351_pll_parameters_t pllb;
} si5351_parameters_t;

typedef struct
{
    si5351_parameters_t current_parameters;
    si5351_children_t outputs[8];
    uint8_t num_registered_clocks;
} si5351_data_t;

typedef struct
{
    si5351_output_parameters_t current_parameters;
} si5351_output_data_t;

// Other internally used structs

typedef struct
{
    bool sys_init;
    bool plla_loss_of_lock;
    bool pllb_loss_of_lock;
    bool clkin_loss_of_signal;
    bool xtal_loss_of_signal;
    uint8_t revision_id : 2;
} si5351_status_t;

#endif // ZEPHYR_DRIVERS_CLOCK_CONTROL_SI5351_H_
