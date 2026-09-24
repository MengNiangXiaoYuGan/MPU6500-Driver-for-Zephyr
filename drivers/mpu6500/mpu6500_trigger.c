/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Change some code
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "mpu6500.h"

LOG_MODULE_DECLARE(MPU6500, CONFIG_SENSOR_LOG_LEVEL);

int mpu6500_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct mpu6500_data *drv_data = dev->data;
	const struct mpu6500_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		LOG_ERR("DEVICE tree not set ini gpio");
		return -ENOTSUP;
	}

	switch (trig->type) 
	{
		case SENSOR_TRIG_DATA_READY:
			/* enable data ready interrupt */
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
						  MPU6500_DRDY_EN) < 0) {
				LOG_ERR("Failed to enable data ready interrupt.");
				return -EIO;
			}
			LOG_DBG("set triger mode data ready");
			break;
		case SENSOR_TRIG_MOTION:
			/*set pwr manager*/
			// if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT1,
			// 			  MPU6500_CYCLE|MPU6500_SLEEP_EN|MPU6500_GYRO_STANDBY,0) < 0) {
			// 	LOG_ERR("Failed to setup power manager");
			// 	return -EIO;
			// }
			// if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT2,
			// 			  MPU6500_DIS_XA|MPU6500_DIS_YA|MPU6500_DIS_ZA|MPU6500_DIS_XG|MPU6500_DIS_YG|MPU6500_DIS_ZG,0b000111) < 0) {
			// 	LOG_ERR("Failed to setup power manager");
			// 	return -EIO;
			// }
			/*set accel LPF setting to 184 hz Bandwidth*/
			// if (i2c_reg_update_byte_dt(&cfg->i2c, MPU6500_REG_ACCEL_CFG2,
			// 			  MPU6500_ACCEL_FEHOICE_B,0) < 0) {
			// 	LOG_ERR("Failed to setup accel_fehoice_b");
			// 	return -EIO;
			// }

			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_ACCEL_CFG2,
						  1) < 0) {
				LOG_ERR("Failed to setup accel dlpf cfg");
				return -EIO;
			}
			/*only this two can init motion interrupt*/

			/*enable motion interrupt*/
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
						  MPU6500_WOM_EN) < 0) {
				LOG_ERR("Failed to enable motion throd interrupt.");
				return -EIO;
			}
			/*enable accel hardware intelligence*/
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_MOT_DETECT_CTRL,
						MPU6500_ACCEL_INTER_EN | MPU6500_ACCEL_INTER_MODE) < 0)
			{
				LOG_ERR("Failed to setup mnotion detect ctrl.");
				return -EIO;
			}
			/*use for set low power cycle to wake up*/
			/*set frequency of wake up*/
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_LP_ACCEL_ODR,
						  (MPU6500_LPOSC_CLKSEL)&CONFIG_MPU6500_WAKE_UP_FREQUENCY_VALUE) < 0) {
				LOG_ERR("Failed to setup mot detect ctrl to inter mode");
				return -EIO;
			}
			/*enable cycle mode*/
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT1,
						  MPU6500_CYCLE) < 0) {
				LOG_ERR("Failed to setup power manager");
				return -EIO;
			}
			LOG_DBG("set triger mode motion");
			break;
		case SENSOR_TRIG_OVERFLOW:
			/*enable FIFO overflow interrupt*/
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
						  MPU6500_FIFO_OVERFLOW_EN) < 0) {
				LOG_ERR("Failed to enable FIFO overflow interrupt.");
				return -EIO;
			}
			LOG_DBG("set triger mode FIFO overflow");
			break;
		default:
			/*disable all intrrupt*/
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_ACCEL_CFG2,
						  0) < 0) {
				LOG_ERR("Failed to setup accel dlpf cfg");
				return -EIO;
			}
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
						  0) < 0) {
				LOG_ERR("Failed to disable motion throd interrupt.");
				return -EIO;
			}
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_MOT_DETECT_CTRL,
						0) < 0)
			{
				LOG_ERR("Failed to setup mnotion detect ctrl.");
				return -EIO;
			}
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_LP_ACCEL_ODR,
						  0) < 0) {
				LOG_ERR("Failed to setup mot detect ctrl to inter mode");
				return -EIO;
			}
			if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT1,
						  0) < 0) {
				LOG_ERR("Failed to setup power manager");
				return -EIO;
			}
			if (gpio_pin_interrupt_configure_dt(&cfg->int_gpio, 
					GPIO_INT_DISABLE))
			{
				LOG_ERR("Failed to disable gpio interrupt");
				return -EIO;
			}
			LOG_DBG("disbale all triger");
			return 0;
			break;
	}

	// uint8_t status = 0;
	// i2c_reg_read_byte_dt(&cfg->i2c, MPU6500_REG_MOT_DETECT_CTRL, &status);
	// LOG_DBG("DBG : read detect c = %d",status);
	// k_msleep(100);

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, 
					GPIO_INT_DISABLE);

	drv_data->callback_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->callback_trigger = trig;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

/*
	@brief this fuction use for set attr to motion trigger
	@brief this is api function
	@note val only use interger value,form 0 - 1020 (mg)
*/
int mpu6500_attr_set(const struct device *dev,
						enum sensor_channel chan,
						enum sensor_attribute attr,
						const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ACCEL_XYZ)
	{
		LOG_ERR("channle select error, only use SENSOR_CHAN_ACCEL_XYZ");
		return -ENOTSUP;

	}

	const struct mpu6500_config *cfg = (const struct mpu6500_config *)dev->config;
	struct mpu6500_data *data = (struct mpu6500_data *)dev->data;
	
	switch (attr)
	{
	case SENSOR_ATTR_UPPER_THRESH:
		if (val->val1 < 0 || val->val2 > 1020)
		{
			LOG_ERR("attr value is not fit to registe");
			return -ENOTSUP;
		}
		/* thans this value to reg*/
		uint8_t value = val->val1 / 4;
		/*value lsb is 4mg*/
		if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_WOM_THR,
					  value) < 0) {
			LOG_ERR("Failed to write MOW_THR reg.");
			return -EIO;
		}
		data->attr_val = *val;
		break;
	default:
		LOG_ERR("this attr is not support");
		return -ENOTSUP;
		break;
	}

	return 0;
}

int mpu6500_attr_get(const struct device *dev,
					enum sensor_channel chan,
					enum sensor_attribute attr,
					struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ACCEL_XYZ)
	{
		LOG_ERR("channle select error, only use SENSOR_CHAN_ACCEL_XYZ");
		return -ENOTSUP;

	}

	struct mpu6500_data *data = (struct mpu6500_data *)dev->data;
	
	switch (attr)
	{
	case SENSOR_ATTR_UPPER_THRESH:
		*val = data->attr_val;
		break;
	default:
		LOG_ERR("this attr is not support");
		return -ENOTSUP;
		break;
	}

	return 0;
}

static void mpu6500_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct mpu6500_data *drv_data =
		CONTAINER_OF(cb, struct mpu6500_data, gpio_cb);
	const struct mpu6500_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_MPU6500_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_MPU6500_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif

	// if (drv_data->callback_handler != NULL) {
	// 	drv_data->callback_handler(dev,
	// 				     drv_data->callback_trigger);
	// }

	// gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
	// 				GPIO_INT_EDGE_TO_ACTIVE);
	// if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
	// 			  0) < 0) {
	// 	LOG_ERR("Failed to disable interrupt.");
	// }
}

static void mpu6500_thread_cb(const struct device *dev)
{
	struct mpu6500_data *drv_data = dev->data;
	const struct mpu6500_config *cfg = dev->config;

	if (drv_data->callback_handler != NULL) {
		drv_data->callback_handler(dev,
					     drv_data->callback_trigger);
	}
	// /*switch triger to disable triger set*/
	// switch (drv_data->callback_trigger->type)
	// {
	// case SENSOR_TRIG_DATA_READY:
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
	// 				  0) < 0) {
	// 		LOG_ERR("Failed to disable data ready interrupt.");
	// 	}
	// 	LOG_DBG("disable data ready triger");
	// 	break;
	// case SENSOR_TRIG_MOTION:
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_ACCEL_CFG2,
	// 				  0) < 0) {
	// 		LOG_ERR("Failed to setup accel dlpf cfg");
	// 	}
	// 	/*only this two can init motion interrupt*/

	// 	/*enable motion interrupt*/
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
	// 				  0) < 0) {
	// 		LOG_ERR("Failed to disable motion throd interrupt.");
	// 	}
	// 	/*enable accel hardware intelligence*/
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_MOT_DETECT_CTRL,
	// 				0) < 0)
	// 	{
	// 		LOG_ERR("Failed to setup mnotion detect ctrl.");
	// 	}
	// 	/*use for set low power cycle to wake up*/
	// 	/*set frequency of wake up*/
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_LP_ACCEL_ODR,
	// 				  0) < 0) {
	// 		LOG_ERR("Failed to setup mot detect ctrl to inter mode");
	// 	}
	// 	/*enable cycle mode*/
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_PWR_MGMT1,
	// 				  0) < 0) {
	// 		LOG_ERR("Failed to setup power manager");
	// 	}
	// 	LOG_DBG("disbale motion triger");
	// 	break;
	// case SENSOR_TRIG_FIFO_FULL:
	// 	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6500_REG_INT_EN,
	// 				  0) < 0) {
	// 		LOG_ERR("Failed to disable FIFO overflow interrupt.");
	// 	}
	// 	LOG_DBG("disbale fifo overflow trigger");
	// 	break;
	// default:
	// 	break;
	// }

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_MPU6500_TRIGGER_OWN_THREAD
static void mpu6500_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct mpu6500_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		mpu6500_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_MPU6500_TRIGGER_GLOBAL_THREAD
static void mpu6500_work_cb(struct k_work *work)
{
	struct mpu6500_data *drv_data =
		CONTAINER_OF(work, struct mpu6500_data, work);

	mpu6500_thread_cb(drv_data->dev);
}
#endif

int mpu6500_init_interrupt(const struct device *dev)
{
	struct mpu6500_data *drv_data = dev->data;
	const struct mpu6500_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->int_gpio.port);
		return -ENODEV;
	}
	LOG_DBG("gpio init ok");

	drv_data->dev = dev;

	gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	LOG_DBG("int gpio configure ok");

	gpio_init_callback(&drv_data->gpio_cb,
			   mpu6500_gpio_callback,
			   BIT(cfg->int_gpio.pin));
	LOG_DBG("INT gpio init callback ok");

	if (gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}
	LOG_DBG("gpio add callback fuction ok");

#if defined(CONFIG_MPU6500_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_MPU6500_THREAD_STACK_SIZE,
			mpu6500_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_MPU6500_THREAD_PRIORITY),
			0, K_NO_WAIT);
	LOG_DBG("init thread ok, k_sem ok");
#elif defined(CONFIG_MPU6500_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = mpu6500_work_cb;
	LOG_DBG("init work q ok");
#endif

	return 0;
}
