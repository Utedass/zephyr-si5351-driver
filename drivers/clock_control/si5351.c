/*
 * Copyright (c) 2018 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT skyworks_si5351

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control/si5351.h>
#include <zephyr/kernel.h>
#include <string.h>

#include "si5351.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_si5351, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static int si5351_write_configuration(const struct device *dev)
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
    uint8_t pll_cfg = data->current_parameters.clkin_div << 6 |
                      data->current_parameters.pllb.clock_source << 3 |
                      data->current_parameters.plla.clock_source << 2;
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_PLL_CFG_ADR, pll_cfg))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set the XTAL load
    uint8_t xtal_load = data->current_parameters.xtal_load << 6 | 0x12; // Magic given from AN619
    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_XTAL_LOAD_ADR, xtal_load))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // === Set clock output specific settings ===
    si5351_output_parameters_t const *clock_parameters;

    // Set clock output phase offsets, only supported for the first 6 clock outputs
    for (int i = 0; i < 6; i++)
    {
        if (!data->outputs[i].output_present)
        {
            memset(&i2c_burst_buffer[i * SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE], 0, SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE);
            continue;
        }
        clock_parameters = data->outputs[i].current_parameters;

        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_PHASE_OFFSET_X_SIZE] = clock_parameters->phase_offset;
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
        clock_parameters = data->outputs[i].current_parameters;

        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P3M_OFFSET] = (clock_parameters->p3 & 0x00ff00) >> 8;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P3L_OFFSET] = (clock_parameters->p3 & 0x0000ff) >> 0;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P1H_OFFSET] = clock_parameters->r << 4 | (clock_parameters->divide_by_four ? 0x3 : 0x0) << 2 | (clock_parameters->p1 & 0x030000) >> 16;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P1M_OFFSET] = (clock_parameters->p1 & 0x00ff00) >> 8;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P1L_OFFSET] = (clock_parameters->p1 & 0x0000ff) >> 0;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P3HP2H_OFFSET] = (clock_parameters->p3 & 0x0f0000) >> (16 - 4) | (clock_parameters->p2 & 0x0f0000) >> 16;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P2M_OFFSET] = (clock_parameters->p2 & 0x00ff00) >> 8;
        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_X_SIZE + SI5351_REG_CLK_OUT_X_P2L_OFFSET] = (clock_parameters->p2 & 0x0000ff) >> 0;
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
        clock_parameters = data->outputs[i].current_parameters;

        i2c_burst_buffer[i * SI5351_REG_CLK_OUT_CTRL_SIZE] = clock_parameters->powered_up << 7 |
                                                             clock_parameters->integer_mode << 6 |
                                                             clock_parameters->multisynth_source << 5 |
                                                             clock_parameters->invert << 4 |
                                                             clock_parameters->clock_source << 2 |
                                                             clock_parameters->drive_strength << 0;
    }
    if (i2c_burst_write_dt(i2c, SI5351_REG_CLK_OUT_CTRL_ADR_BASE, i2c_burst_buffer, 8 * SI5351_REG_CLK_OUT_CTRL_SIZE))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Set PLL multisynth settings
    i2c_burst_buffer[SI5351_REG_PLL_X_P3M_OFFSET] = (data->current_parameters.plla.p3 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P3L_OFFSET] = (data->current_parameters.plla.p3 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1H_OFFSET] = (data->current_parameters.plla.p1 & 0x030000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1M_OFFSET] = (data->current_parameters.plla.p1 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P1L_OFFSET] = (data->current_parameters.plla.p1 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_P3HP2H_OFFSET] = (data->current_parameters.plla.p3 & 0x0f0000) >> (16 - 4) | (data->current_parameters.plla.p2 & 0x0f0000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_P2M_OFFSET] = (data->current_parameters.plla.p2 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_P2L_OFFSET] = (data->current_parameters.plla.p2 & 0x0000ff) >> 0;

    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3M_OFFSET] = (data->current_parameters.pllb.p3 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3L_OFFSET] = (data->current_parameters.pllb.p3 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1H_OFFSET] = (data->current_parameters.pllb.p1 & 0x030000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1M_OFFSET] = (data->current_parameters.pllb.p1 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P1L_OFFSET] = (data->current_parameters.pllb.p1 & 0x0000ff) >> 0;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P3HP2H_OFFSET] = (data->current_parameters.pllb.p3 & 0x0f0000) >> (16 - 4) | (data->current_parameters.pllb.p2 & 0x0f0000) >> 16;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P2M_OFFSET] = (data->current_parameters.pllb.p2 & 0x00ff00) >> 8;
    i2c_burst_buffer[SI5351_REG_PLL_X_SIZE + SI5351_REG_PLL_X_P2L_OFFSET] = (data->current_parameters.pllb.p2 & 0x0000ff) >> 0;

    if (i2c_burst_write_dt(i2c, SI5351_REG_PLL_X_ADR_BASE, i2c_burst_buffer, SI5351_REG_PLL_X_SIZE * 2))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    // Reset PLLs
    if (si5351_reset_pll(dev, si5351_pll_index_a | si5351_pll_index_b))
    {
        return -EIO;
    }

    // Enable outputs
    i2c_burst_buffer[0] = 0x00;
    for (int i = 0; i < 8; i++)
    {
        if (!data->outputs[i].output_present)
        {
            continue;
        }
        clock_parameters = data->outputs[i].current_parameters;

        i2c_burst_buffer[0] |= clock_parameters->output_enabled << i;
    }
    if (i2c_burst_write_dt(i2c, SI5351_REG_OEB_ADR, i2c_burst_buffer, 1))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }

    return 0;
}

int si5351_get_parameters(const struct device *dev, si5351_parameters_t *parameters)
{
    return 0;
}

int si5351_set_parameters(const struct device *dev, si5351_parameters_t const *parameters)
{
    si5351_data_t *data = dev->data;

    memcpy(&data->current_parameters, parameters, sizeof(si5351_parameters_t));

    return 0;
}

int si5351_reset_pll(const struct device *dev, si5351_pll_index_t pll)
{
    si5351_config_t const *cfg = dev->config;
    struct i2c_dt_spec const *i2c = &cfg->i2c;

    uint8_t pll_reset_register = 0;
    pll_reset_register |= (pll & si5351_pll_index_b) ? 0x80 : 0x00;
    pll_reset_register |= (pll & si5351_pll_index_a) ? 0x20 : 0x00;

    if (i2c_reg_write_byte_dt(i2c, SI5351_REG_PLL_RESET_ADR, pll_reset_register))
    {
        LOG_ERR("Could not write to device");
        return -EIO;
    }
    return 0;
}

int si5351_output_get_parameters(const struct device *dev, si5351_output_parameters_t *parameters)
{
    return 0;
}

int si5351_output_set_parameters(const struct device *dev, si5351_output_parameters_t const *parameters)
{
    si5351_output_data_t *data = dev->data;

    memcpy(&data->current_parameters, parameters, sizeof(si5351_output_parameters_t));

    return 0;
}

static int si5351_output_on(const struct device *dev, clock_control_subsys_t subsys)
{
    // const si5351_output_config_t *cfg = dev->config;
    LOG_DBG("SI5351_on entered");
    return 0;
}

static int si5351_output_off(const struct device *dev, clock_control_subsys_t subsys)
{
    // const si5351_output_config_t *cfg = dev->config;
    LOG_DBG("SI5351_off entered");
    return 0;
}

static int si5351_setup(const struct device *dev)
{
    const si5351_config_t *cfg = dev->config;

    uint8_t status;
    if (i2c_reg_read_byte_dt(&cfg->i2c, SI5351_REG_STATUS_ADR, &status))
    {
        LOG_ERR("Could not read from device at 0x%" PRIX16, cfg->i2c.addr);
        return -EIO;
    }

    return 0;
}

// Convert from device tree parameter to correct C representation of parameters
static inline int parse_output_dt_parameters(si5351_output_dt_config_t const *default_config_in, si5351_output_parameters_t *config_out)
{
    config_out->output_enabled = default_config_in->output_enabled ? si5351_output_output_enabled : si5351_output_output_disabled;

    config_out->powered_up = default_config_in->powered_up ? si5351_output_powered_up : si5351_output_powered_down;

    config_out->integer_mode = default_config_in->integer_mode ? si5351_output_integer_mode_enabled : si5351_output_integer_mode_disabled;

    switch (default_config_in->multisynth_source)
    {
    case 0:
        config_out->multisynth_source = si5351_output_multisynth_source_plla;
        break;
    case 1:
        config_out->multisynth_source = si5351_output_multisynth_source_pllb;
        break;
    default:
        LOG_ERR("Invalid argument: multisynth_source: %d", (int)default_config_in->multisynth_source);
        return -EINVAL;
    }

    config_out->invert = default_config_in->invert ? si5351_output_invert_enabled : si5351_output_invert_disabled;

    switch (default_config_in->clock_source)
    {
    case 0:
        config_out->clock_source = si5351_output_clk_source_xtal;
        break;
    case 1:
        config_out->clock_source = si5351_output_clk_source_clkin;
        break;
    case 2:
        config_out->clock_source = si5351_output_clk_source_multisynth;
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
        config_out->r = si5351_output_r_1;
        break;
    case 2:
        config_out->r = si5351_output_r_2;
        break;
    case 4:
        config_out->r = si5351_output_r_4;
        break;
    case 8:
        config_out->r = si5351_output_r_8;
        break;
    case 16:
        config_out->r = si5351_output_r_16;
        break;
    case 32:
        config_out->r = si5351_output_r_32;
        break;
    case 64:
        config_out->r = si5351_output_r_64;
        break;
    case 128:
        config_out->r = si5351_output_r_128;
        break;
    default:
        LOG_ERR("Invalid argument: clock_source: %d", (int)default_config_in->clock_source);
        return -EINVAL;
    }

    config_out->divide_by_four = default_config_in->divide_by_four;

    config_out->phase_offset = default_config_in->phase_offset;

    return 0;
}

// Registers a clock output to the si5351 driver
// When the final clock output has been registered, this calls the device initialization
static int si5351_register_output(const struct device *parent, const struct device *child)
{
    const si5351_config_t *cfg = parent->config;
    const si5351_output_config_t *clock_cfg = child->config;

    si5351_data_t *data = parent->data;

    data->outputs[clock_cfg->output_index].output_present = true;
    data->outputs[clock_cfg->output_index].current_parameters = child->data;
    data->num_registered_clocks++;

    if (data->num_registered_clocks == cfg->num_okay_clocks)
    {
        // All clocks registered, perform chip initialization
        LOG_DBG("All outputs registered, performing chip initialization..");
        si5351_write_configuration(parent);
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

    if (parse_output_dt_parameters(&cfg->dt_config, &data->current_parameters) < 0)
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
            "phase_offset: %d",
            (int)data->current_parameters.output_enabled,
            (int)data->current_parameters.powered_up,
            (int)data->current_parameters.integer_mode,
            (int)data->current_parameters.multisynth_source,
            (int)data->current_parameters.invert,
            (int)data->current_parameters.clock_source,
            (int)data->current_parameters.drive_strength,
            (int)data->current_parameters.p1,
            (int)data->current_parameters.p2,
            (int)data->current_parameters.p3,
            (int)data->current_parameters.r,
            (int)data->current_parameters.divide_by_four,
            (int)data->current_parameters.phase_offset);

    si5351_register_output(cfg->parent, dev);

    LOG_DBG("si5351_output_%d initialized", cfg->output_index);
    return 0;
}

static int si5351_parse_dt_parameters(si5351_dt_config_t const *default_config_in, si5351_parameters_t *config_out)
{
    switch (default_config_in->clkin_div)
    {
    case 1:
        config_out->clkin_div = si5351_clkin_div_1;
        break;
    case 2:
        config_out->clkin_div = si5351_clkin_div_2;
        break;
    case 4:
        config_out->clkin_div = si5351_clkin_div_4;
        break;
    case 8:
        config_out->clkin_div = si5351_clkin_div_8;
        break;
    default:
        LOG_ERR("Invalid argument: clkin_div: %d", (int)default_config_in->clkin_div);
        return -EINVAL;
    };

    switch (default_config_in->xtal_load)
    {
    case 6:
        config_out->xtal_load = si5351_xtal_load_6pf;
        break;
    case 8:
        config_out->xtal_load = si5351_xtal_load_8pf;
        break;
    case 10:
        config_out->xtal_load = si5351_xtal_load_10pf;
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

    if (si5351_setup(dev) < 0)
    {
        LOG_ERR("Failed to setup device!");
        return -EIO;
    }

    si5351_parse_dt_parameters(&cfg->dt_config, &data->current_parameters);

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
            (int)data->current_parameters.clkin_div,
            (int)data->current_parameters.xtal_load,
            (int)data->current_parameters.plla.clock_source,
            (int)data->current_parameters.plla.p1,
            (int)data->current_parameters.plla.p2,
            (int)data->current_parameters.plla.p3,
            (int)data->current_parameters.pllb.clock_source,
            (int)data->current_parameters.pllb.p1,
            (int)data->current_parameters.pllb.p2,
            (int)data->current_parameters.pllb.p3);

    LOG_DBG("si5351 driver loaded for device at 0x%" PRIX16, cfg->i2c.addr);

    return 0;
}

static DEVICE_API(clock_control, si5351_output_driver_api) = {
    .on = si5351_output_on,
    .off = si5351_output_off,
    .async_on = NULL,
    .get_rate = NULL,
    .get_status = NULL,
    .set_rate = NULL,
    .configure = NULL,
};

// Macro called once per clock output defined in the device tree
// This parses the options given in the device tree and assigns them to the
// dt_config struct. The output_init function will copy this to the
// data->current_config during runtime initialization
#define SI5351_OUTPUT_INIT(child_node_id)                                       \
    static si5351_output_data_t si5351_output_data##child_node_id;              \
    static const si5351_output_config_t si5351_output_config##child_node_id = { \
        .parent = DEVICE_DT_GET(DT_PARENT(child_node_id)),                      \
        .output_index = DT_REG_ADDR(child_node_id),                             \
        .dt_config = {                                                          \
            .output_enabled = DT_PROP(child_node_id, output_enabled),           \
            .powered_up = DT_PROP(child_node_id, powered_up),                   \
            .integer_mode = DT_PROP(child_node_id, integer_mode),               \
            .multisynth_source = DT_ENUM_IDX(child_node_id, multisynth_source), \
            .invert = DT_PROP(child_node_id, invert),                           \
            .clock_source = DT_ENUM_IDX(child_node_id, clock_source),           \
            .drive_strength = DT_PROP(child_node_id, drive_strength),           \
            .p1 = DT_PROP(child_node_id, p1),                                   \
            .p2 = DT_PROP(child_node_id, p2),                                   \
            .p3 = DT_PROP(child_node_id, p3),                                   \
            .r = DT_PROP(child_node_id, r),                                     \
            .divide_by_four = DT_PROP(child_node_id, divide_by_four),           \
            .phase_offset = DT_PROP(child_node_id, phase_offset),               \
        },                                                                      \
    };                                                                          \
    DEVICE_DT_DEFINE(child_node_id, &si5351_output_init, NULL,                  \
                     &si5351_output_data##child_node_id,                        \
                     &si5351_output_config##child_node_id, POST_KERNEL,         \
                     SI5351_INIT_PRIORITY, &si5351_output_driver_api)

// Macro, called once per si5351 instance
// This parses the options given in the device tree and assigns them to the
// dt_config struct. The output_init function will copy this to the
// data->current_config during runtime initialization
#define SI5351_INIT(inst)                                                  \
    static si5351_data_t si5351_data_##inst;                               \
    static const si5351_config_t si5351_config_##inst = {                  \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
        .dt_config = {                                                     \
            .clkin_div = DT_INST_PROP(inst, clkin_div),                    \
            .xtal_load = DT_INST_PROP(inst, xtal_load),                    \
            .plla = {                                                      \
                .clock_source = DT_INST_ENUM_IDX(inst, plla_clock_source), \
                .p1 = DT_INST_PROP(inst, plla_p1),                         \
                .p2 = DT_INST_PROP(inst, plla_p2),                         \
                .p3 = DT_INST_PROP(inst, plla_p3),                         \
            },                                                             \
            .pllb = {                                                      \
                .clock_source = DT_INST_ENUM_IDX(inst, pllb_clock_source), \
                .p1 = DT_INST_PROP(inst, pllb_p1),                         \
                .p2 = DT_INST_PROP(inst, pllb_p2),                         \
                .p3 = DT_INST_PROP(inst, pllb_p3),                         \
            },                                                             \
        },                                                                 \
        .num_okay_clocks = DT_INST_CHILD_NUM_STATUS_OKAY(inst),            \
    };                                                                     \
    DEVICE_DT_INST_DEFINE(inst, &si5351_init, NULL,                        \
                          &si5351_data_##inst,                             \
                          &si5351_config_##inst, POST_KERNEL,              \
                          SI5351_INIT_PRIORITY,                            \
                          NULL);                                           \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SI5351_OUTPUT_INIT)

// Macro to call SI5351 for each instance in the device tree, provided by Zephyrs devicetree.h
DT_INST_FOREACH_STATUS_OKAY(SI5351_INIT)
