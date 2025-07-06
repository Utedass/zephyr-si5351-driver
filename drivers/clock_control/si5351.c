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
    const si5351_output_config_t *cfg = dev->config;
    LOG_DBG("SI5351_on entered");
    return 0;
}

static int si5351_output_off(const struct device *dev, clock_control_subsys_t subsys)
{
    const si5351_output_config_t *cfg = dev->config;
    LOG_DBG("SI5351_off entered");
    return 0;
}

static int si5351_setup(const struct device *dev)
{
    const si5351_config_t *config = dev->config;

    uint8_t status;
    if (i2c_reg_read_byte_dt(&config->i2c, SI5351_REG_ADR_STATUS, &status))
    {
        LOG_ERR("Could not read device at 0x%" PRIX16, config->i2c.addr);
        return -EIO;
    }

    // Use DT_PROP(something, prop_name) to get properties. But what should >something< be?

    return 0;
}

static int si5351_output_init(const struct device *dev)
{
    const si5351_output_config_t *cfg = dev->config;

    if (!device_is_ready(cfg->parent))
    {
        LOG_ERR("Parent device not ready");
        return -ENODEV;
    }

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
#define SI5351_OUTPUT_INIT(child_node_id)                                       \
    static si5351_output_data_t si5351_output_data##child_node_id;              \
    static const si5351_output_config_t si5351_output_config##child_node_id = { \
        .parent = DEVICE_DT_GET(DT_PARENT(child_node_id)),                      \
        .output_index = DT_REG_ADDR(child_node_id),                             \
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
