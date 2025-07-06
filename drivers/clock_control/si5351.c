/*
 * Copyright (c) 2018 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT skyworks_si5351

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control/si5351.h>

#include "si5351.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_si5351, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

int si5351_dummy(const struct device *dev)
{
    LOG_DBG("SI5351_dummy entered");
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
    if (i2c_reg_read_byte_dt(&cfg->i2c, SI5351_REG_ADR_STATUS, &status))
    {
        LOG_ERR("Could not read device at 0x%" PRIX16, cfg->i2c.addr);
        return -EIO;
    }

    // Use DT_PROP(something, prop_name) to get properties. But what should >something< be?

    return 0;
}

// Convert from device tree parameter to correct C representation of parameters
static inline int parse_default_parameters(si5351_output_default_config_t const *default_config_in, si5351_output_parameters_t *config_out)
{
    switch (default_config_in->powered)
    {
    case 0:
        config_out->powered = si5351_output_powered_down;
        break;
    case 1:
        config_out->powered = si5351_output_powered_up;
        break;
    default:
        LOG_ERR("Invalid argument: powered: %d", (int)default_config_in->powered);
        return -EINVAL;
    }

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
    config_out->r0 = default_config_in->r0;

    switch (default_config_in->r0)
    {
    case 1:
        config_out->r0 = si5351_output_r0_1;
        break;
    case 2:
        config_out->r0 = si5351_output_r0_2;
        break;
    case 4:
        config_out->r0 = si5351_output_r0_4;
        break;
    case 8:
        config_out->r0 = si5351_output_r0_8;
        break;
    case 16:
        config_out->r0 = si5351_output_r0_16;
        break;
    case 32:
        config_out->r0 = si5351_output_r0_32;
        break;
    case 64:
        config_out->r0 = si5351_output_r0_64;
        break;
    case 128:
        config_out->r0 = si5351_output_r0_128;
        break;
    default:
        LOG_ERR("Invalid argument: clock_source: %d", (int)default_config_in->clock_source);
        return -EINVAL;
    }

    config_out->divide_by_four = default_config_in->divide_by_four;

    config_out->phase_offset = default_config_in->phase_offset;

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

    if (parse_default_parameters(&cfg->default_config, &data->current_parameters) < 0)
    {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    LOG_DBG("powered: %d\r\n"
            "integer_mode: %d\r\n"
            "multisynth_source: %d\r\n"
            "invert: %d\r\n"
            "clock_source: %d\r\n"
            "drive_strength: %d\r\n"
            "p1: %d\r\n"
            "p2: %d\r\n"
            "p3: %d\r\n"
            "r0: %d\r\n"
            "divide_by_four: %d\r\n"
            "phase_offset: %d",
            (int)data->current_parameters.powered,
            (int)data->current_parameters.integer_mode,
            (int)data->current_parameters.multisynth_source,
            (int)data->current_parameters.invert,
            (int)data->current_parameters.clock_source,
            (int)data->current_parameters.drive_strength,
            (int)data->current_parameters.p1,
            (int)data->current_parameters.p2,
            (int)data->current_parameters.p3,
            (int)data->current_parameters.r0,
            (int)data->current_parameters.divide_by_four,
            (int)data->current_parameters.phase_offset);

    LOG_DBG("si5351_output_%d initialized", cfg->output_index);
    return 0;
}

static int si5351_init(const struct device *dev)
{
    const si5351_config_t *cfg;
    const si5351_data_t *data;

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
// default_config struct. The output_init function will copy this to the
// data->current_config during runtime initialization
#define SI5351_OUTPUT_INIT(child_node_id)                                       \
    static si5351_output_data_t si5351_output_data##child_node_id;              \
    static const si5351_output_config_t si5351_output_config##child_node_id = { \
        .parent = DEVICE_DT_GET(DT_PARENT(child_node_id)),                      \
        .output_index = DT_REG_ADDR(child_node_id),                             \
        .default_config = {                                                     \
            .powered = DT_ENUM_IDX(child_node_id, powered),                     \
            .integer_mode = DT_PROP(child_node_id, integer_mode),               \
            .multisynth_source = DT_ENUM_IDX(child_node_id, multisynth_source), \
            .invert = DT_PROP(child_node_id, invert),                           \
            .clock_source = DT_ENUM_IDX(child_node_id, clock_source),           \
            .drive_strength = DT_PROP(child_node_id, drive_strength),           \
            .p1 = DT_PROP_OR(child_node_id, p1, 0),                             \
            .p2 = DT_PROP_OR(child_node_id, p2, 0),                             \
            .p3 = DT_PROP_OR(child_node_id, p3, 1), /* Avoid divide-by-zero */  \
            .r0 = DT_PROP(child_node_id, r0),                                   \
            .divide_by_four = DT_PROP(child_node_id, divide_by_four),           \
            .phase_offset = DT_PROP(child_node_id, phase_offset),               \
        },                                                                      \
    };                                                                          \
    DEVICE_DT_DEFINE(child_node_id, &si5351_output_init, NULL,                  \
                     &si5351_output_data##child_node_id,                        \
                     &si5351_output_config##child_node_id, POST_KERNEL,         \
                     SI5351_INIT_PRIORITY, &si5351_output_driver_api)

// Macro, called once per instance
#define SI5351_INIT(inst)                                     \
    static si5351_data_t si5351_data_##inst;                  \
    static const si5351_config_t si5351_config_##inst = {     \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                    \
    };                                                        \
    DEVICE_DT_INST_DEFINE(inst, &si5351_init, NULL,           \
                          &si5351_data_##inst,                \
                          &si5351_config_##inst, POST_KERNEL, \
                          SI5351_INIT_PRIORITY,               \
                          NULL);                              \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SI5351_OUTPUT_INIT)

// Macro to call SI5351 for each instance in the device tree, provided by Zephyrs devicetree.h
DT_INST_FOREACH_STATUS_OKAY(SI5351_INIT)
