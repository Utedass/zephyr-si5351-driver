/*
 * Copyright (c) 2018 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT skyworks_si5351

#include "si5351.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control/si5351.h>
#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_si5351, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

int si5351_get_status(const struct device *dev, si5351_status_t *status)
{
    si5351_config_t const *cfg = dev->config;
    struct i2c_dt_spec const *i2c = &cfg->i2c;

    uint8_t status_register;
    if (i2c_reg_read_byte_dt(i2c, SI5351_REG_STATUS_ADR, &status_register))
    {
        LOG_ERR("Could not read status register");
        return -EIO;
    }

    status->sys_init = (status_register & 0x01) != 0;
    status->plla_loss_of_lock = (status_register & 0x02) != 0;
    status->pllb_loss_of_lock = (status_register & 0x04) != 0;
    status->clkin_loss_of_signal = (status_register & 0x08) != 0;
    status->xtal_loss_of_signal = (status_register & 0x10) != 0;
    status->revision_id = (status_register >> 5) & 0x03;

    return 0;
}

int si5351_tune_pll(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_parameters_t const *parameters)
{
    si5351_config_t const *cfg = dev->config;
    si5351_data_t *data = dev->data;
    struct i2c_dt_spec const *i2c = &cfg->i2c;

    uint8_t i2c_burst_buffer[SI5351_REG_PLL_X_SIZE * 2];

    // Set PLL multisynth settings
    if (pll_mask & SI5351_PLL_MASK_A)
    {
        memcpy(&data->plla, parameters, sizeof(si5351_pll_parameters_t));
    }
    if (pll_mask & SI5351_PLL_MASK_B)
    {
        memcpy(&data->pllb, parameters, sizeof(si5351_pll_parameters_t));
    }

    i2c_burst_buffer[SI5351_REG_PLL_X_P3M_OFFSET] = (data->plla.p3 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P3L_OFFSET] = (data->plla.p3 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1H_OFFSET] = (data->plla.p1 & 0x030000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1M_OFFSET] = (data->plla.p1 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1L_OFFSET] = (data->plla.p1 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_P3HP2H_OFFSET] = (data->plla.p3 & 0x0f0000) >> (16 - 4) | (data->plla.p2 & 0x0f0000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_P2M_OFFSET] = (data->plla.p2 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P2L_OFFSET] = (data->plla.p2 & 0x0000ff) >> 0;

    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3M_OFFSET] = (data->pllb.p3 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3L_OFFSET] = (data->pllb.p3 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1H_OFFSET] = (data->pllb.p1 & 0x030000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1M_OFFSET] = (data->pllb.p1 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1L_OFFSET] = (data->pllb.p1 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3HP2H_OFFSET] = (data->pllb.p3 & 0x0f0000) >> (16 - 4) | (data->pllb.p2 & 0x0f0000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P2M_OFFSET] = (data->pllb.p2 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P2L_OFFSET] = (data->pllb.p2 & 0x0000ff) >> 0;

    if (i2c_burst_write_dt(i2c, SI5351_REG_PLL_X_ADR_BASE, i2c_burst_buffer, SI5351_REG_PLL_X_SIZE * 2))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    return 0;
}

static int si5351_write_oeb(const struct device *dev)
{
    si5351_config_t const *cfg = dev->config;
    si5351_data_t *data = dev->data;
    struct i2c_dt_spec const *i2c = &cfg->i2c;
    si5351_output_data_t const *clock_data;

    // Update OEB register
    uint8_t oeb_register = 0x00;
    for (int i = 0; i < 8; i++)
    {
        if (!data->outputs[i].output_present)
        {
            // Disable unused outputs by default
            oeb_register |= 1 << i;
            continue;
        }
        clock_data = data->outputs[i].clock_output_data;

        oeb_register |= clock_data->output_enabled << i;
    }
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_OEB_ADR, oeb_register))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    return 0;
}

int si5351_set_output(const struct device *dev, uint8_t output_index, si5351_output_output_t state)
{
    si5351_data_t *data = dev->data;

    if (output_index >= 8)
    {
        LOG_ERR("Invalid output index: %d", output_index);
        return -EINVAL;
    }

    if (!data->outputs[output_index].output_present)
    {
        LOG_ERR("Output %d is not present", output_index);
        return -ENODEV;
    }

    data->outputs[output_index].clock_output_data->output_enabled = state;

    return si5351_write_oeb(dev);
}

// Convert from device tree parameter to correct C representation of parameters
static inline int parse_output_dt_parameters(si5351_output_dt_config_t const *default_config_in, si5351_output_data_t *config_out)
{
    config_out->output_enabled = default_config_in->output_enabled ? SI5351_OUTPUT_OUTPUT_ENABLED : SI5351_OUTPUT_OUTPUT_DISABLED;

    config_out->powered_up = default_config_in->powered_up ? SI5351_OUTPUT_POWERED_UP : SI5351_OUTPUT_POWERED_DOWN;

    config_out->integer_mode = default_config_in->integer_mode ? SI5351_OUTPUT_INTEGER_MODE_ENABLED : SI5351_OUTPUT_INTEGER_MODE_DISABLED;

    switch (default_config_in->multisynth_source)
    {
    case 0:
        config_out->multisynth_source = SI5351_OUTPUT_MULTISYNTH_SOURCE_PLLA;
        break;
    case 1:
        config_out->multisynth_source = SI5351_OUTPUT_MULTISYNTH_SOURCE_PLLB;
        break;
    default:
        LOG_ERR("Invalid argument: multisynth_source: %d", (int)default_config_in->multisynth_source);
        return -EINVAL;
    }

    config_out->invert = default_config_in->invert ? SI5351_OUTPUT_INVERT_ENABLED : SI5351_OUTPUT_INVERT_DISABLED;

    switch (default_config_in->clock_source)
    {
    case 0:
        config_out->clock_source = SI5351_OUTPUT_CLK_SOURCE_XTAL;
        break;
    case 1:
        config_out->clock_source = SI5351_OUTPUT_CLK_SOURCE_CLKIN;
        break;
    case 2:
        config_out->clock_source = SI5351_OUTPUT_CLK_SOURCE_MULTISYNTH;
        break;
    default:
        LOG_ERR("Invalid argument: clock_source: %d", (int)default_config_in->clock_source);
        return -EINVAL;
    }

    config_out->drive_strength = default_config_in->drive_strength;
    config_out->p1 = default_config_in->p1;
    config_out->p2 = default_config_in->p2;
    config_out->p3 = default_config_in->p3;
    config_out->r = default_config_in->r;

    switch (default_config_in->r)
    {
    case 1:
        config_out->r = SI5351_OUTPUT_R_1;
        break;
    case 2:
        config_out->r = SI5351_OUTPUT_R_2;
        break;
    case 4:
        config_out->r = SI5351_OUTPUT_R_4;
        break;
    case 8:
        config_out->r = SI5351_OUTPUT_R_8;
        break;
    case 16:
        config_out->r = SI5351_OUTPUT_R_16;
        break;
    case 32:
        config_out->r = SI5351_OUTPUT_R_32;
        break;
    case 64:
        config_out->r = SI5351_OUTPUT_R_64;
        break;
    case 128:
        config_out->r = SI5351_OUTPUT_R_128;
        break;
    default:
        LOG_ERR("Invalid argument: clock_source: %d", (int)default_config_in->clock_source);
        return -EINVAL;
    }

    config_out->divide_by_four = default_config_in->divide_by_four;

    config_out->phase_offset = default_config_in->phase_offset;

    return 0;
}

static int si5351_parse_dt_parameters(si5351_dt_config_t const *default_config_in, si5351_data_t *config_out)
{
    switch (default_config_in->clkin_div)
    {
    case 1:
        config_out->clkin_div = SI5351_CLKIN_DIV_1;
        break;
    case 2:
        config_out->clkin_div = SI5351_CLKIN_DIV_2;
        break;
    case 4:
        config_out->clkin_div = SI5351_CLKIN_DIV_4;
        break;
    case 8:
        config_out->clkin_div = SI5351_CLKIN_DIV_8;
        break;
    default:
        LOG_ERR("Invalid argument: clkin_div: %d", (int)default_config_in->clkin_div);
        return -EINVAL;
    };

    switch (default_config_in->xtal_load)
    {
    case 6:
        config_out->xtal_load = SI5351_XTAL_LOAD_6PF;
        break;
    case 8:
        config_out->xtal_load = SI5351_XTAL_LOAD_8PF;
        break;
    case 10:
        config_out->xtal_load = SI5351_XTAL_LOAD_10PF;
        break;
    default:
        LOG_ERR("Invalid argument: xtal_load: %d", (int)default_config_in->xtal_load);
        return -EINVAL;
    };

    config_out->plla.p1 = default_config_in->plla.p1;
    config_out->plla.p2 = default_config_in->plla.p2;
    config_out->plla.p3 = default_config_in->plla.p3;

    config_out->pllb.p1 = default_config_in->pllb.p1;
    config_out->pllb.p2 = default_config_in->pllb.p2;
    config_out->pllb.p3 = default_config_in->pllb.p3;

    return 0;
}

int si5351_reapply_configuration(const struct device *dev)
{
    si5351_config_t const *cfg = dev->config;
    si5351_data_t *data = dev->data;
    struct i2c_dt_spec const *i2c = &cfg->i2c;

    // Prepare a burst buffer for the maximum number of clocks, disregard variants for now
    uint8_t i2c_burst_buffer[SI5351_REG_CLK_OUT_X_SIZE * 8];

    // Disable OEB inputs
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_OEB_MASK_ADR, 0xff))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Disable OEB
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_OEB_ADR, 0xff))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Reset PLL settings
    if (i2c_burst_write_dt(i2c, SI5351_REG_PLL_X_ADR_BASE, 0, SI5351_REG_PLL_X_SIZE * 2))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Power down all output drivers
    for (int i = 0; i < 8; i++)
    {
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_CTRL_SIZE] = 0x80;
    }
    if (i2c_burst_write_dt(i2c, SI5351_REG_CLK_OUT_CTRL_ADR_BASE, i2c_burst_buffer, SI5351_REG_CLK_OUT_CTRL_SIZE * 8))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Mask all interrupts for now
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_INTERRUPT_MASK_ADR, 0xf8))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    k_busy_wait(1000 * 100);

    // Clear any sticy interrupt bits
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_INTERRUPT_ADR, 0x00))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set PLL settings
    uint8_t pll_cfg = data->clkin_div << 6 |
                      data->pllb.clock_source << 3 |
                      data->plla.clock_source << 2;
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_PLL_CFG_ADR, pll_cfg))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set the XTAL load
    uint8_t xtal_load = data->xtal_load << 6 | 0x12; // Magic given from AN619
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_XTAL_LOAD_ADR, xtal_load))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // === Set clock output specific settings ===
    si5351_output_data_t const *clock_output_data;

    // Set clock output phase offsets, only supported for the first 6 clock outputs
    for (int i = 0; i < 6; i++)
    {
        if (!data->outputs[i].output_present)
        {
            memset(&i2c_burst_buffer[i * SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE], 0, SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE);
            continue;
        }
        clock_output_data = data->outputs[i].clock_output_data;

        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE] = clock_output_data->phase_offset;
    }
    if (i2c_burst_write_dt(i2c, SI5351_REG_CLK_OUT_PHASE_OFFSET_X_ADR_BASE, i2c_burst_buffer, 6 * SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set clock output multisynths
    for (int i = 0; i < 8; i++)
    {
        if (!data->outputs[i].output_present)
        {
            memset(&i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE], 0, SI5351_REG_CLK_OUT_X_SIZE);
            continue;
        }
        clock_output_data = data->outputs[i].clock_output_data;

        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P3M_OFFSET] = (clock_output_data->p3 & 0x00ff00) >> 8;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P3L_OFFSET] = (clock_output_data->p3 & 0x0000ff) >> 0;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P1H_OFFSET] = clock_output_data->r << 4 | (clock_output_data->divide_by_four ? 0x3 : 0x0) << 2 | (clock_output_data->p1 & 0x030000) >> 16;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P1M_OFFSET] = (clock_output_data->p1 & 0x00ff00) >> 8;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P1L_OFFSET] = (clock_output_data->p1 & 0x0000ff) >> 0;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P3HP2H_OFFSET] = (clock_output_data->p3 & 0x0f0000) >> (16 - 4) | (clock_output_data->p2 & 0x0f0000) >> 16;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P2M_OFFSET] = (clock_output_data->p2 & 0x00ff00) >> 8;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P2L_OFFSET] = (clock_output_data->p2 & 0x0000ff) >> 0;
    }
    if (i2c_burst_write_dt(i2c, SI5351_REG_CLK_OUT_X_ADR_BASE, i2c_burst_buffer, 8 * SI5351_REG_CLK_OUT_X_SIZE))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set clock config
    for (int i = 0; i < 8; i++)
    {
        if (!data->outputs[i].output_present)
        {
            memset(&i2c_burst_buffer[i * SI5351_REG_CLK_OUT_CTRL_SIZE], 0, SI5351_REG_CLK_OUT_CTRL_SIZE);
            continue;
        }
        clock_output_data = data->outputs[i].clock_output_data;

        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_CTRL_SIZE] = clock_output_data->powered_up << 7 |
                                                             clock_output_data->integer_mode << 6 |
                                                             clock_output_data->multisynth_source << 5 |
                                                             clock_output_data->invert << 4 |
                                                             clock_output_data->clock_source << 2 |
                                                             clock_output_data->drive_strength << 0;
    }
    if (i2c_burst_write_dt(i2c, SI5351_REG_CLK_OUT_CTRL_ADR_BASE, i2c_burst_buffer, 8 * SI5351_REG_CLK_OUT_CTRL_SIZE))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set PLL multisynth settings
    i2c_burst_buffer[SI5351_REG_PLL_X_P3M_OFFSET] = (data->plla.p3 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P3L_OFFSET] = (data->plla.p3 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1H_OFFSET] = (data->plla.p1 & 0x030000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1M_OFFSET] = (data->plla.p1 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1L_OFFSET] = (data->plla.p1 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_P3HP2H_OFFSET] = (data->plla.p3 & 0x0f0000) >> (16 - 4) | (data->plla.p2 & 0x0f0000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_P2M_OFFSET] = (data->plla.p2 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P2L_OFFSET] = (data->plla.p2 & 0x0000ff) >> 0;

    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3M_OFFSET] = (data->pllb.p3 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3L_OFFSET] = (data->pllb.p3 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1H_OFFSET] = (data->pllb.p1 & 0x030000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1M_OFFSET] = (data->pllb.p1 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1L_OFFSET] = (data->pllb.p1 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3HP2H_OFFSET] = (data->pllb.p3 & 0x0f0000) >> (16 - 4) | (data->pllb.p2 & 0x0f0000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P2M_OFFSET] = (data->pllb.p2 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P2L_OFFSET] = (data->pllb.p2 & 0x0000ff) >> 0;

    if (i2c_burst_write_dt(i2c, SI5351_REG_PLL_X_ADR_BASE, i2c_burst_buffer, SI5351_REG_PLL_X_SIZE * 2))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Reset PLLs
    if (si5351_pll_soft_reset(dev, SI5351_PLL_MASK_A | SI5351_PLL_MASK_B))
    {
        return -EIO;
    }

    // Update OEB register
    if (si5351_write_oeb(dev))
    {
        return -EIO;
    }

    return 0;
}

int si5351_apply_dt_settings(const struct device *dev)
{
    si5351_config_t const *cfg = dev->config;
    si5351_data_t *data = dev->data;

    si5351_parse_dt_parameters(&cfg->dt_config, data);

    for (int i = 0; i < si5351_model_features[cfg->dt_config.model].number_of_outputs; i++)
    {

        if (data->outputs[i].output_present)
        {
            si5351_output_config_t const *output_cfg = data->outputs[i].clock_output_cfg;
            si5351_output_data_t *output_data = data->outputs[i].clock_output_data;
            parse_output_dt_parameters(&output_cfg->dt_config, output_data);
        }
    }

    return 0;
}

int si5351_set_clkin(const struct device *dev, uint32_t frequency, si5351_clkin_div_t div)
{
    return 0;
}

int si5351_get_revision_number(const struct device *dev, si5351_revision_number_t *revision_number)
{
    return 0;
}

bool si5351_is_xtal_running(const struct device *dev)
{
    return false;
}

bool si5351_is_clkin_running(const struct device *dev)
{
    return false;
}

int si5351_pll_soft_reset(const struct device *dev, si5351_pll_mask_t pll)
{
    si5351_config_t const *cfg = dev->config;
    struct i2c_dt_spec const *i2c = &cfg->i2c;

    uint8_t pll_reset_register = 0;
    pll_reset_register |= (pll & SI5351_PLL_MASK_B) ? 0x80 : 0x00;
    pll_reset_register |= (pll & SI5351_PLL_MASK_A) ? 0x20 : 0x00;

    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_PLL_RESET_ADR, pll_reset_register))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }
    return 0;
}

int si5351_pll_set_frequency(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_source_t source, si5351_frequency_t frequency)
{
    return 0;
}

int si5351_pll_set_multiplier(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_ratio_t multiplier)
{
    return 0;
}

int si5351_pll_set_multiplier_abc(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t a, uint32_t b, uint32_t c)
{
    return 0;
}

int si5351_pll_set_multiplier_parameters(const struct device *dev, si5351_pll_mask_t pll_mask, si5351_pll_parameters_t parameters)
{
    return 0;
}

int si5351_pll_set_multiplier_integer(const struct device *dev, si5351_pll_mask_t pll_mask, uint32_t multiplier)
{
    return 0;
}

int si5351_pll_set_divider_fixed(const struct device *dev, si5351_pll_mask_t pll_mask, bool is_fixed)
{
    return 0;
}

int si5351_pll_get_frequency(const struct device *dev, si5351_pll_index_t pll_index, si5351_frequency_t *frequency)
{
    return 0;
}

int si5351_pll_get_multiplier(const struct device *dev, si5351_pll_index_t pll_index, si5351_ratio_t *multiplier)
{
    return 0;
}

bool si5351_pll_is_fixed(const struct device *dev, si5351_pll_index_t pll_index)
{
    return false;
}

bool si5351_pll_is_locked(const struct device *dev, si5351_pll_index_t pll_index)
{
    return false;
}

int si5351_output_set_source(const struct device *dev, si5351_output_mask_t output_mask, si5351_output_source_t source)
{
    return 0;
}

int si5351_output_set_multisynth_source(const struct device *dev, si5351_output_mask_t output_mask, si5351_output_multisynth_source_t source)
{
    return 0;
}

int si5351_output_set_frequency(const struct device *dev, si5351_output_mask_t output_mask, si5351_frequency_t frequency)
{
    return 0;
}

int si5351_output_set_divider(const struct device *dev, si5351_output_mask_t output_mask, si5351_ratio_t divider)
{
    return 0;
}

int si5351_output_set_divider_abc(const struct device *dev, si5351_output_mask_t output_mask, uint32_t a, uint32_t b, uint32_t c)
{
    return 0;
}

int si5351_output_set_divider_parameters(const struct device *dev, si5351_output_mask_t output_mask, uint32_t p1, uint32_t p2, uint32_t p3)
{
    return 0;
}

int si5351_output_set_divider_integer(const struct device *dev, si5351_output_mask_t output_mask, uint8_t integer)
{
    return 0;
}

int si5351_output_set_divider_fixed(const struct device *dev, si5351_output_mask_t output_mask, bool is_fixed)
{
    return 0;
}

int si5351_output_set_powered_down(const struct device *dev, si5351_output_mask_t output_mask, bool is_powered_down)
{
    return 0;
}

int si5351_output_set_output_enabled(const struct device *dev, si5351_output_mask_t output_mask, bool is_enabled)
{
    return 0;
}

int si5351_output_set_output_enable_mask(const struct device *dev, si5351_output_mask_t output_mask, bool is_masked)
{
    return 0;
}

int si5351_output_set_inverted(const struct device *dev, si5351_output_mask_t output_mask, bool is_inverted)
{
    return 0;
}

int si5351_output_set_phase_offset_ps(const struct device *dev, si5351_output_mask_t output_mask, uint32_t pico_seconds)
{
    return 0;
}

int si5351_output_set_phase_offset_val(const struct device *dev, si5351_output_mask_t output_mask, uint8_t val)
{
    return 0;
}

int si5351_output_get_frequency(const struct device *dev, si5351_output_index_t output_index, si5351_frequency_t *frequency)
{
    return 0;
}

int si5351_output_get_divider(const struct device *dev, si5351_output_index_t output_index, si5351_ratio_t *divider)
{
    return 0;
}

// Registers a clock output to the si5351 driver
// When the final clock output of a si5351 chip has been registered, this calls the device initialization
static int si5351_register_output(const struct device *parent, const struct device *child)
{
    const si5351_config_t *parent_cfg = parent->config;
    si5351_data_t *parent_data = parent->data;
    const si5351_output_config_t *child_cfg = child->config;
    si5351_output_data_t *child_data = child->data;

    parent_data->outputs[child_cfg->dt_config.output_index].output_present = true;
    parent_data->outputs[child_cfg->dt_config.output_index].clock_output_cfg = child_cfg;
    parent_data->outputs[child_cfg->dt_config.output_index].clock_output_data = child_data;
    parent_data->num_registered_clocks++;

    if (parent_data->num_registered_clocks == parent_cfg->num_okay_clocks)
    {
        // All clocks registered, perform chip initialization
        LOG_DBG("All outputs registered, performing chip initialization..");
        si5351_reapply_configuration(parent);
    }

    return 0;
}

static int si5351_output_init(const struct device *dev)
{
    const si5351_output_config_t *cfg = dev->config;
    si5351_output_data_t *data = dev->data;

    if (!device_is_ready(cfg->parent))
    {
        LOG_ERR("Parent device not ready");
        return -ENODEV;
    }

    if (parse_output_dt_parameters(&cfg->dt_config, data) < 0)
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }

    LOG_DBG("output_enabled: %d\r\n"
            "powered_up: %d\r\n"
            "integer_mode: %d\r\n"
            "multisynth_source: %d\r\n"
            "invert: %d\r\n"
            "clock_source: %d\r\n"
            "drive_strength: %d\r\n"
            "p1: %d\r\n"
            "p2: %d\r\n"
            "p3: %d\r\n"
            "r: %d\r\n"
            "divide_by_four: %d\r\n"
            "phase_offset: %d\r\n"
            "fixed_divider: %d",
            (int)data->output_enabled,
            (int)data->powered_up,
            (int)data->integer_mode,
            (int)data->multisynth_source,
            (int)data->invert,
            (int)data->clock_source,
            (int)data->drive_strength,
            (int)data->p1,
            (int)data->p2,
            (int)data->p3,
            (int)data->r,
            (int)data->divide_by_four,
            (int)data->phase_offset,
            (int)data->fixed_divider);

    si5351_register_output(cfg->parent, dev);

    LOG_DBG("si5351_output_%d initialized", cfg->dt_config.output_index);
    return 0;
}

static int si5351_init(const struct device *dev)
{
    const si5351_config_t *cfg;
    si5351_data_t *data;

    cfg = dev->config;
    data = dev->data;

    if (!device_is_ready(cfg->i2c.bus))
    {
        LOG_ERR("Bus device is not ready");
        return -ENODEV;
    }

    si5351_status_t status;
    if (si5351_get_status(dev, &status))
    {
        LOG_ERR("Failed to setup device at 0x%" PRIX16, cfg->i2c.addr);
        return -EIO;
    }

    si5351_parse_dt_parameters(&cfg->dt_config, data);

    LOG_DBG("clkin_div: %d\r\n"
            "xtal_load: %d\r\n"
            "plla.clock_source: %d\r\n"
            "plla.p1: %d\r\n"
            "plla.p2: %d\r\n"
            "plla.p3: %d\r\n"
            "pllb.clock_source: %d\r\n"
            "pllb.p1: %d\r\n"
            "pllb.p2: %d\r\n"
            "pllb.p3: %d\r\n",
            (int)data->clkin_div,
            (int)data->xtal_load,
            (int)data->plla.clock_source,
            (int)data->plla.p1,
            (int)data->plla.p2,
            (int)data->plla.p3,
            (int)data->pllb.clock_source,
            (int)data->pllb.p1,
            (int)data->pllb.p2,
            (int)data->pllb.p3);

    LOG_DBG("si5351 driver loaded for device at 0x%" PRIX16, cfg->i2c.addr);

    return 0;
}

// ====== Clock Control API Functions begin ======
static int si5351_output_on(const struct device *dev, clock_control_subsys_t subsys)
{
    const si5351_output_config_t *cfg = dev->config;
    si5351_output_data_t *data = dev->data;
    LOG_DBG("SI5351_on entered");

    data->output_enabled = SI5351_OUTPUT_OUTPUT_ENABLED;

    si5351_write_oeb(cfg->parent);
    return 0;
}

static int si5351_output_off(const struct device *dev, clock_control_subsys_t subsys)
{
    const si5351_output_config_t *cfg = dev->config;
    si5351_output_data_t *data = dev->data;
    LOG_DBG("SI5351_off entered");

    data->output_enabled = SI5351_OUTPUT_OUTPUT_DISABLED;

    si5351_write_oeb(cfg->parent);
    return 0;
}
// ====== Clock Control API Functions end ======

// Clock control API structure
static DEVICE_API(clock_control, si5351_output_driver_api) = {
    .on = si5351_output_on,
    .off = si5351_output_off,
    .async_on = NULL,
    .get_rate = NULL,
    .get_status = NULL,
    .set_rate = NULL,
    .configure = NULL,
};

// Helper macros to ensure mutually exclusive properties are not in the device tree

#define DT_NODE_HAS_PROPS_2(node_id, p1, p2) \
    (DT_NODE_HAS_PROP(node_id, p1) || DT_NODE_HAS_PROP(node_id, p2))

#define DT_NODE_HAS_PROPS_3(node_id, p1, p2, p3) \
    (DT_NODE_HAS_PROPS_2(node_id, p1, p2) || DT_NODE_HAS_PROP(node_id, p3))

#define DT_NODE_HAS_PROPS_4(node_id, p1, p2, p3, p4) \
    (DT_NODE_HAS_PROPS_3(node_id, p1, p2, p3) || DT_NODE_HAS_PROP(node_id, p4))

#define DT_NODE_HAS_PROPS_5(node_id, p1, p2, p3, p4, p5) \
    (DT_NODE_HAS_PROPS_4(node_id, p1, p2, p3, p4) || DT_NODE_HAS_PROP(node_id, p5))

#define _DT_NODE_HAS_PROPS_CHOOSER(_1, _2, _3, _4, _5, NAME, ...) NAME

// One through five properties can be checked for existence
// DT_NODE_HAS_PROPS(node_id, prop1, prop2 ...)
#define DT_NODE_HAS_PROPS(node_id, ...)             \
    _DT_NODE_HAS_PROPS_CHOOSER(__VA_ARGS__,         \
                               DT_NODE_HAS_PROPS_5, \
                               DT_NODE_HAS_PROPS_4, \
                               DT_NODE_HAS_PROPS_3, \
                               DT_NODE_HAS_PROPS_2, \
                               DT_NODE_HAS_PROP)(node_id, __VA_ARGS__)

#define DT_INST_NODE_HAS_PROPS(inst, ...) \
    DT_NODE_HAS_PROPS(DT_DRV_INST(inst), __VA_ARGS__)

#define UNPACK_GROUP_IMPL(...) __VA_ARGS__
#define UNPACK_GROUP(args) UNPACK_GROUP_IMPL args

#define DT_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(node_id, a, b)                                                    \
    BUILD_ASSERT(!(DT_NODE_HAS_PROPS(node_id, UNPACK_GROUP(a)) && DT_NODE_HAS_PROPS(node_id, UNPACK_GROUP(b))), \
                 "DT error: si5351 cannot have Properties from groupA " #a " and groupB " #b " set")

#define DT_INST_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(inst, a, b) \
    DT_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(DT_DRV_INST(inst), a, b)

// Macro called once per clock output defined in the device tree
// This parses the options given in the device tree and assigns them to the
// dt_config struct. The output_init function will copy this to the
// data->current_config during runtime initialization
#define SI5351_OUTPUT_INIT(child_node_id)                                                                                     \
    static si5351_output_data_t si5351_output_data##child_node_id;                                                            \
    static const si5351_output_config_t si5351_output_config##child_node_id = {                                               \
        .parent = DEVICE_DT_GET(DT_PARENT(child_node_id)),                                                                    \
        .dt_config = {                                                                                                        \
            .output_index = DT_REG_ADDR(child_node_id),                                                                       \
            .output_enabled = DT_PROP(child_node_id, output_enabled),                                                         \
            .powered_up = DT_PROP(child_node_id, powered_up),                                                                 \
            .integer_mode = DT_PROP(child_node_id, integer_mode),                                                             \
            .clock_source = DT_ENUM_IDX(child_node_id, clock_source),                                                         \
            .multisynth_source = DT_ENUM_IDX(child_node_id, multisynth_source),                                               \
            .drive_strength = DT_PROP(child_node_id, drive_strength),                                                         \
            .invert = DT_PROP(child_node_id, invert),                                                                         \
            .fixed_divider = DT_PROP(child_node_id, fixed_divider),                                                           \
                                                                                                                              \
            .using_frequency = DT_NODE_HAS_PROPS(inst, frequency, frequency_fractional),                                      \
            .frequency = DT_PROP_OR(inst, frequency, 0),                                                                      \
            .frequency_fractional = DT_PROP_OR(inst, frequency_fractional, 0),                                                \
                                                                                                                              \
            .using_multiplier = DT_NODE_HAS_PROPS(inst, a, b, c),                                                             \
            .a = DT_PROP_OR(inst, a, 0),                                                                                      \
            .b = DT_PROP_OR(inst, b, 0),                                                                                      \
            .c = DT_PROP_OR(inst, c, 0),                                                                                      \
                                                                                                                              \
            .using_multiplier_parameters = DT_NODE_HAS_PROPS(child_node_id, p1, p2, p3, r, divide_by_four),                   \
            .p1 = DT_PROP_OR(child_node_id, p1, 0),                                                                           \
            .p2 = DT_PROP_OR(child_node_id, p2, 0),                                                                           \
            .p3 = DT_PROP_OR(child_node_id, p3, 0),                                                                           \
            .r = DT_PROP_OR(child_node_id, r, 0),                                                                             \
            .divide_by_four = DT_PROP(child_node_id, divide_by_four),                                                         \
                                                                                                                              \
            .using_phase_offset = DT_NODE_HAS_PROP(child_node_id, phase_offset),                                              \
            .phase_offset = DT_PROP_OR(child_node_id, phase_offset, 0),                                                       \
                                                                                                                              \
            .using_phase_offset_ps = DT_NODE_HAS_PROP(child_node_id, phase_offset_ps),                                        \
            .phase_offset_ps = DT_PROP_OR(child_node_id, phase_offset_ps, 0),                                                 \
        },                                                                                                                    \
    };                                                                                                                        \
    DT_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(child_node_id, (frequency, frequency_fractional), (a, b, v));                       \
    DT_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(child_node_id, (frequency, frequency_fractional), (p1, p2, p3, r, divide_by_four)); \
    DT_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(child_node_id, (a, b, c), (p1, p2, p3, r, divide_by_four));                         \
    DT_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(child_node_id, (phase_offset), (phase_offset_ps));                                  \
    DEVICE_DT_DEFINE(child_node_id, &si5351_output_init, NULL,                                                                \
                     &si5351_output_data##child_node_id,                                                                      \
                     &si5351_output_config##child_node_id, POST_KERNEL,                                                       \
                     SI5351_INIT_PRIORITY, &si5351_output_driver_api)

// Macro, called once per si5351 instance
// This parses the options given in the device tree and assigns them to the
// dt_config struct. The output_init function will copy this to the
// data->current_config during runtime initialization
#define SI5351_INIT(inst)                                                                                                       \
    static si5351_data_t si5351_data_##inst;                                                                                    \
    static const si5351_config_t si5351_config_##inst = {                                                                       \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                                                                      \
        .dt_config = {                                                                                                          \
            .model = DT_INST_ENUM_IDX(inst, model),                                                                             \
            .xtal_frequency = DT_INST_PROP(inst, xtal_frequency),                                                               \
            .xtal_load = DT_INST_PROP(inst, xtal_load),                                                                         \
            .clkin_frequency = DT_INST_PROP_OR(inst, clkin_frequency, 0),                                                       \
            .clkin_div = DT_INST_PROP(inst, clkin_div),                                                                         \
            .plla = {                                                                                                           \
                .clock_source = DT_INST_ENUM_IDX(inst, plla_clock_source),                                                      \
                .fixed_multiplier = DT_INST_PROP(inst, plla_fixed_multiplier),                                                  \
                                                                                                                                \
                .using_frequency = DT_INST_NODE_HAS_PROPS(inst, plla_frequency, plla_frequency_fractional),                     \
                .frequency = DT_INST_PROP_OR(inst, plla_frequency, 0),                                                          \
                .frequency_fractional = DT_INST_PROP_OR(inst, plla_frequency_fractional, 0),                                    \
                                                                                                                                \
                .using_multiplier = DT_INST_NODE_HAS_PROPS(inst, plla_a, plla_b, plla_c),                                       \
                .a = DT_INST_PROP_OR(inst, plla_a, 0),                                                                          \
                .b = DT_INST_PROP_OR(inst, plla_b, 0),                                                                          \
                .c = DT_INST_PROP_OR(inst, plla_c, 0),                                                                          \
                                                                                                                                \
                .using_multiplier_parameters = DT_INST_NODE_HAS_PROPS(inst, plla_p1, plla_p2, plla_p3),                         \
                .p1 = DT_INST_PROP_OR(inst, plla_p1, 0),                                                                        \
                .p2 = DT_INST_PROP_OR(inst, plla_p2, 0),                                                                        \
                .p3 = DT_INST_PROP_OR(inst, plla_p3, 0),                                                                        \
                                                                                                                                \
                .integer_mode = DT_INST_PROP(inst, plla_integer_mode),                                                          \
            },                                                                                                                  \
            .pllb = {                                                                                                           \
                .clock_source = DT_INST_ENUM_IDX(inst, pllb_clock_source),                                                      \
                .fixed_multiplier = DT_INST_PROP(inst, pllb_fixed_multiplier),                                                  \
                                                                                                                                \
                .using_frequency = DT_INST_NODE_HAS_PROPS(inst, pllb_frequency, pllb_frequency_fractional),                     \
                .frequency = DT_INST_PROP_OR(inst, pllb_frequency, 0),                                                          \
                .frequency_fractional = DT_INST_PROP_OR(inst, pllb_frequency_fractional, 0),                                    \
                                                                                                                                \
                .using_multiplier = DT_INST_NODE_HAS_PROPS(inst, pllb_a, pllb_b, pllb_c),                                       \
                .a = DT_INST_PROP_OR(inst, pllb_a, 0),                                                                          \
                .b = DT_INST_PROP_OR(inst, pllb_b, 0),                                                                          \
                .c = DT_INST_PROP_OR(inst, pllb_c, 0),                                                                          \
                                                                                                                                \
                .using_multiplier_parameters = DT_INST_NODE_HAS_PROPS(inst, pllb_p1, pllb_p2, pllb_p3),                         \
                .p1 = DT_INST_PROP_OR(inst, pllb_p1, 0),                                                                        \
                .p2 = DT_INST_PROP_OR(inst, pllb_p2, 0),                                                                        \
                .p3 = DT_INST_PROP_OR(inst, pllb_p3, 0),                                                                        \
                                                                                                                                \
                .integer_mode = DT_INST_PROP(inst, pllb_integer_mode),                                                          \
            },                                                                                                                  \
        },                                                                                                                      \
        .num_okay_clocks = DT_INST_CHILD_NUM_STATUS_OKAY(inst),                                                                 \
    };                                                                                                                          \
    DT_INST_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(inst, (plla_frequency, plla_frequency_fractional), (plla_a, plla_b, plla_c));    \
    DT_INST_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(inst, (plla_frequency, plla_frequency_fractional), (plla_p1, plla_p2, plla_p3)); \
    DT_INST_MUTUALLY_EXCLUSIVE_PROPERTY_GROUPS(inst, (plla_a, plla_b, plla_c), (plla_p1, plla_p2, plla_p3));                    \
    DEVICE_DT_INST_DEFINE(inst, &si5351_init, NULL,                                                                             \
                          &si5351_data_##inst,                                                                                  \
                          &si5351_config_##inst, POST_KERNEL,                                                                   \
                          SI5351_INIT_PRIORITY,                                                                                 \
                          NULL);                                                                                                \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SI5351_OUTPUT_INIT)

// Macro to call SI5351 for each instance in the device tree, provided by Zephyrs devicetree.h
DT_INST_FOREACH_STATUS_OKAY(SI5351_INIT)
