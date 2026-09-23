/*
 * Copyright (c) 2016 Intel Corporation
 * Change some code
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MPU6500_MPU6500_H_
#define ZEPHYR_DRIVERS_SENSOR_MPU6500_MPU6500_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define MPU6500_REG_WHO_AM_I		0x75
#define MPU6500_CHIP_ID				0x70

#define MPU6500_REG_SMPLRT_DIV		0x19

#define MPU6500_REG_GYRO_CFG		0x1B
#define MPU6500_GYRO_FS_SHIFT		3

#define MPU6500_REG_ACCEL_CFG		0x1C
#define MPU6500_ACCEL_FS_SHIFT		3

#define MPU6500_REG_ACCEL_CFG2		0x1D
#define MPU6500_ACCEL_FEHOICE_B		BIT(3)
#define MPU6500_A_DLPF_CFG			BIT(2)|BIT(1)|BIT(0)

#define MPU6500_REG_INT_EN			0x38
#define MPU6500_REG_INT_STATUS		0x3A
#define MPU6500_DRDY_EN				BIT(0)
#define MPU6500_WOM_EN				BIT(6)
#define MPU6500_FIFO_OVERFLOW_EN	BIT(4)
#define MPU6500_FSYNC_INT_EN		BIT(3)

#define MPU6500_REG_LP_ACCEL_ODR	0x1E
#define MPU6500_LPOSC_CLKSEL		BIT(3)|BIT(2)|BIT(1)|BIT(0)

#define MPU6500_REG_WOM_THR			0x1f

#define MPU6500_REG_DATA_START		0x3B

#define MPU6500_REG_MOT_DETECT_CTRL	0X69
#define MPU6500_ACCEL_INTER_EN		BIT(7)
#define MPU6500_ACCEL_INTER_MODE	BIT(6)

#define MPU6500_REG_PWR_MGMT1		0x6B
#define MPU6500_SLEEP_EN			BIT(6)
#define MPU6500_DEVICE_RESET		BIT(7)
#define MPU6500_CYCLE				BIT(5)
#define MPU6500_GYRO_STANDBY		BIT(4)
#define MPU6500_GYRO_TEMP_DIS		BIT(3)
#define MPU6500_CLKSEL				BIT(2)|BIT(1)|BIT(0)

#define MPU6500_REG_PWR_MGMT2		0X6C
#define MPU6500_LP_WAKE_CTRL		BIT(7)|BIT(6)
#define MPU6500_DIS_XA				BIT(5)
#define MPU6500_DIS_YA				BIT(4)
#define MPU6500_DIS_ZA				BIT(3)
#define MPU6500_DIS_XG				BIT(2)
#define MPU6500_DIS_YG				BIT(1)
#define MPU6500_DIS_ZG				BIT(0)


/* measured in degrees/sec x10 to avoid floating point */
static const uint16_t mpu6500_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

struct mpu6500_data {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
	uint16_t accel_sensitivity_shift;

	int16_t temp;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	uint16_t gyro_sensitivity_x10;

#ifdef CONFIG_MPU6500_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	const struct sensor_trigger *callback_trigger;
	sensor_trigger_handler_t callback_handler;

	struct sensor_value attr_val;
#if defined(CONFIG_MPU6500_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MPU6500_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_MPU6500_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_MPU6500_TRIGGER */
};

struct mpu6500_config {
	struct i2c_dt_spec i2c;
	uint8_t accel_fs;
	uint16_t gyro_fs;
	uint8_t smplrt_div;
#ifdef CONFIG_MPU6500_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif /* CONFIG_MPU6500_TRIGGER */
};

#ifdef CONFIG_MPU6500_TRIGGER
int mpu6500_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int mpu6500_attr_set(const struct device *dev,
						enum sensor_channel chan,
						enum sensor_attribute attr,
						const struct sensor_value *val);

int mpu6500_attr_get(const struct device *dev,
						enum sensor_channel chan,
						enum sensor_attribute attr,
						struct sensor_value *val);

int mpu6500_init_interrupt(const struct device *dev);
#endif

#endif /* __SENSOR_MPU6500__ */
