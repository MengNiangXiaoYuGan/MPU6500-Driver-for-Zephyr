/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_mpu6500_i2c

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "mpu6500.h"

LOG_MODULE_REGISTER(MPU6500, CONFIG_SENSOR_LOG_LEVEL);

/* see "Accelerometer Measurements" section from register map description */
static void mpu6500_convert_accel(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_shift)
{
	int64_t conv_val;

	conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Gyroscope Measurements" section from register map description */
static void mpu6500_convert_gyro(struct sensor_value *val, int16_t raw_val,
				 uint16_t sensitivity_x10)
{
	int64_t conv_val;

	conv_val = ((int64_t)raw_val * SENSOR_PI * 10) /
		   (sensitivity_x10 * 180U);
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Temperature Measurement" section from register map description */
static inline void mpu6500_convert_temp(struct sensor_value *val,
					int16_t raw_val)
{
	int64_t tmp_val = (int64_t)raw_val * 1000000;
	tmp_val = (tmp_val / 340) + 36000000;

	val->val1 = tmp_val / 1000000;
	val->val2 = tmp_val % 1000000;
}

static int mpu6500_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mpu6500_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		mpu6500_convert_accel(val, drv_data->accel_x,
				      drv_data->accel_sensitivity_shift);
		mpu6500_convert_accel(val + 1, drv_data->accel_y,
				      drv_data->accel_sensitivity_shift);
		mpu6500_convert_accel(val + 2, drv_data->accel_z,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		mpu6500_convert_accel(val, drv_data->accel_x,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		mpu6500_convert_accel(val, drv_data->accel_y,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		mpu6500_convert_accel(val, drv_data->accel_z,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		mpu6500_convert_gyro(val, drv_data->gyro_x,
				     drv_data->gyro_sensitivity_x10);
		mpu6500_convert_gyro(val + 1, drv_data->gyro_y,
				     drv_data->gyro_sensitivity_x10);
		mpu6500_convert_gyro(val + 2, drv_data->gyro_z,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		mpu6500_convert_gyro(val, drv_data->gyro_x,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		mpu6500_convert_gyro(val, drv_data->gyro_y,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		mpu6500_convert_gyro(val, drv_data->gyro_z,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		mpu6500_convert_temp(val, drv_data->temp);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mpu6500_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct mpu6500_data *drv_data = dev->data;
	const struct mpu6500_config *cfg = dev->config;
	int16_t buf[7];

	if (i2c_burst_read_dt(&cfg->i2c, MPU6500_REG_DATA_START, (uint8_t *)buf,
			      14) < 0) {
		LOG_ERR("Failed to read data sample.");
		return -EIO;
	}

	drv_data->accel_x = sys_be16_to_cpu(buf[0]);
	drv_data->accel_y = sys_be16_to_cpu(buf[1]);
	drv_data->accel_z = sys_be16_to_cpu(buf[2]);
	drv_data->temp = sys_be16_to_cpu(buf[3]);
	drv_data->gyro_x = sys_be16_to_cpu(buf[4]);
	drv_data->gyro_y = sys_be16_to_cpu(buf[5]);
	drv_data->gyro_z = sys_be16_to_cpu(buf[6]);

	return 0;
}

static DEVICE_API(sensor, mpu6500_driver_api) = {
#if CONFIG_MPU6500_TRIGGER
	.trigger_set = mpu6500_trigger_set,
	.attr_set = mpu6500_attr_set,
	.attr_get = mpu6500_attr_get,
#endif
	.sample_fetch = mpu6500_sample_fetch,
	.channel_get = mpu6500_channel_get,
};

int mpu6500_init(const struct device *dev)
{
	struct mpu6500_data *drv_data = dev->data;
	const struct mpu6500_config *cfg = dev->config;
	uint8_t id, i;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}
		/* chip reset */
	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT1, MPU6500_DEVICE_RESET) < 0) {
		LOG_ERR("Failed to reset chip.");
		return -EIO;
	}
	LOG_DBG("CHIP reset now");
	k_msleep(50);

	/* check chip ID */
	if (i2c_reg_read_byte_dt(&cfg->i2c, MPU6500_REG_WHO_AM_I, &id) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	/*check chip id*/
	if (id != MPU6500_CHIP_ID)
	{
		LOG_ERR("Chip id is not MPU6500");
		return -ENODEV;
	}

	/* wake up chip */
	if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT1,
				   MPU6500_SLEEP_EN, 0) < 0) {
		LOG_ERR("Failed to wake up chip.");
		return -EIO;
	}

	/* set accelerometer full-scale range */
	for (i = 0U; i < 4; i++) {
		if (BIT(i+1) == cfg->accel_fs) {
			break;
		}
	}

	if (i == 4U) {
		LOG_ERR("Invalid value for accel full-scale range.");
		return -EINVAL;
	}

	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_ACCEL_CFG,
				  i << MPU6500_ACCEL_FS_SHIFT) < 0) {
		LOG_ERR("Failed to write accel full-scale range.");
		return -EIO;
	}

	drv_data->accel_sensitivity_shift = 14 - i;

	/* set gyroscope full-scale range */
	for (i = 0U; i < 4; i++) {
		if (BIT(i) * 250 == cfg->gyro_fs) {
			break;
		}
	}

	if (i == 4U) {
		LOG_ERR("Invalid value for gyro full-scale range.");
		return -EINVAL;
	}

	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_GYRO_CFG,
				  i << MPU6500_GYRO_FS_SHIFT) < 0) {
		LOG_ERR("Failed to write gyro full-scale range.");
		return -EIO;
	}

	drv_data->gyro_sensitivity_x10 = mpu6500_gyro_sensitivity_x10[i];

	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_SMPLRT_DIV,
				  cfg->smplrt_div) < 0) {
		LOG_ERR("Failed to write samplerate divider.");
		return -EIO;
	}

#ifdef CONFIG_MPU6500_TRIGGER
	if (cfg->int_gpio.port) {
		if (mpu6500_init_interrupt(dev) < 0) {
			LOG_DBG("Failed to initialize interrupts.");
			return -EIO;
		}
	}
#endif

	return 0;
}

#define MPU6500_DEFINE(inst)									\
	static struct mpu6500_data mpu6500_data_##inst;						\
												\
	static const struct mpu6500_config mpu6500_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.accel_fs = DT_INST_PROP(inst, accel_fs),					\
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),						\
		.smplrt_div = DT_INST_PROP(inst, smplrt_div),					\
		IF_ENABLED(CONFIG_MPU6500_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mpu6500_init, NULL,					\
			      &mpu6500_data_##inst, &mpu6500_config_##inst,			\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &mpu6500_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(MPU6500_DEFINE)
