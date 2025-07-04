/*
 * Copyright (c) 2018 Jonatan Gezelius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT skyworks_si5351

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2c.h>

#include "si5351.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_si5351, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static int si5351_on(const struct device *dev, clock_control_subsys_t subsys)
{
    LOG_DBG("SSI5351_on entered");
    return 0;
}

static int si5351_off(const struct device *dev, clock_control_subsys_t subsys)
{
    LOG_DBG("SSI5351_off entered");
    return 0;
}

static int si5351_setup(const struct device *dev)
{
    const struct si5351_config *config = dev->config;

    uint8_t status;
    if (i2c_reg_read_byte_dt(&config->i2c, SI5351_REG_ADR_STATUS, &status))
    {
        LOG_ERR("Could not read device at 0x%" PRIX16, config->i2c.addr);
        return -EIO;
    }

    return 0;
}

static int si5351_init(const struct device *dev)
{
    const struct si5351_config *cfg;

    cfg = dev->config;

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

static DEVICE_API(clock_control, si5351_driver_api) = {
    .on = si5351_on,
    .off = si5351_off,
    .async_on = NULL,
    .get_rate = NULL,
    .get_status = NULL,
    .set_rate = NULL,
    .configure = NULL,
};

#define SI5351_INIT(inst)                                      \
    static struct si5351_config si5351_config_##inst = {       \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                     \
    };                                                         \
    DEVICE_DT_DEFINE(DT_DRV_INST(inst), &si5351_init, NULL,    \
                     NULL, &si5351_config_##inst, POST_KERNEL, \
                     CONFIG_SENSOR_INIT_PRIORITY,              \
                     &si5351_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SI5351_INIT)
