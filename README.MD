 # 基于Zephyr操作系统的MPU6500-IMU驱动库

 ## 简介
 - 该库基于Zephyr自带的MPU6050库进行拙劣的模仿，保持了MPU6050库内读取和`DATA_READY`触发功能的状态下，添加了以下功能
 - 1. 添加了更多的触发方式，例如`FIFO_OVERFLOW`,以及`MOTION`触发，触发模式是锁存的，启动触发后将无法取触发（我也不知道为什么zephyr的sensor trigger set 居然没有取消触发的选项，很奇怪，但是没关系，除了驱动支持的触发方式外，输入其他的触发方式都会清除所有触发，可以用这种方式将触发清除掉。
 - 2. 在motion触发的选项中，根据mpu6500的数据手册设定了触发，比如运动触发条件下，设备启动了`Cycle`模式，在这个模式下，陀螺仪会被关闭，调用`sensor_sample_fetch`函数同步后所有轴的陀螺仪数据都是0，如果需要获取陀螺仪数据，必须关闭触发才能正常运行，需要根据实际情况进行调试。
  
 ## 驱动逻辑
 - 该驱动的运行逻辑与`zephyr`的`MPU6050`库一致，用户可以正常使用`sensor_sample_fetch`以及`sensor_channel_get`等函数，但是触发部分该驱动进行了一些修改，在保证原本的逻辑不会被破坏的情况下，添加了以下内容
 - 1. 添加了`MOTION`运动触发的逻辑，在加速度计变化到一定程度的状态下，驱动所定义的`int`引脚将会拉高一个脉冲，这个脉冲可以唤醒设备，进入*gpio*中断，并且根据*Kconfig*的设置,使用全局线程*工作队列*`MPU6500_TRIGGER_GLOBAL_THREAD`或者独立的线程`MPU6500_TRIGGER_OWN_THREAD`进行回调函数调用，如果使用独立线程进行调用，`MPU6500_THREAD_STACK_SIZE`和`MPU6500_THREAD_PRIORITY`将会生效，否则这两个设置没有意义。
 - 2. 添加了取消触发的逻辑，当前可使用的触发逻辑为

|触发类型|触发内容 |
| :-----: | :----- |
|`SENSOR_TRIG_DATA_READY`|当原始数据刷新时，进行一次触发|
|`SENSOR_TRIG_MOTION`|当检测到加速度计的值与前一次采样相差超过阈值时触发|
|`SENSOR_TRIG_OVERFLOW`|当检测到FIFO满时进行一次触发|
- 3. 加速度触发的阈值使用`sensor_attr_set`函数进行设置，设置值为物理值，单位*mg*，需要注意的时，值的范围为`0-1020`,`1LSB=4mg`.
- 4. FIFO满触发当前版本无用，因为所有的数据都是直接在原始数据寄存器采集的，并且启动这个触发的时候也不会让数据转存到FIFO里，之后的版本应该会对该功能进行开发。
  
  ## 关于MPU6500
  - MPU6500的运动触发，在没有启动`Cycle`模式的状态下十分容易抽风，并且夹杂着陀螺仪错误的状态，当前所能了解到的信息，建议将运动触发功能和低功耗模式一起使用，如果需要使用运动模式触发后在连续采集的工况运行，建议设置触发并且调用完回调函数，确实被触发后调用`sensor_trigger_set`关闭触发，之后在连续使用`sensor_sample_fetch`进行采样。以下是一个简单的程序思路实例
  ~~~~~
        void mpu_call_back (const struct device *dev,
                            const struct sensor_trigger *trigger)
        {
           /* your code */
            flag++;
        }

        /*****************in main********************************/
        /*set attr*/
        struct sensor_value attr = {
            .val1 = 4*/*your value*/,
        };
        ret = sensor_attr_set(mpu,SENSOR_CHAN_ACCEL_XYZ,SENSOR_ATTR_UPPER_THRESH,&attr);
        if (ret)
        {
            printk("error : can not use this setup,code %d\n",ret);
            return ret;
        }
        /*set triger*/
        struct sensor_trigger tri = {
            .chan = SENSOR_CHAN_ACCEL_XYZ,
            .type = SENSOR_TRIG_MOTION,
        };
        ret = sensor_trigger_set(mpu,&tri,mpu_call_back);
        if (ret)
        {
            printk("error: can not set triger for mpu 6500, code %d\n",ret);
            return ret;
        }

        while (1)
        {
            if(flag >= 20)
            {
                /*set triger disbale*/
                struct sensor_trigger tri = {
                    .chan = SENSOR_CHAN_ACCEL_XYZ,
                    .type = SENSOR_TRIG_PRIV_START, //disable trigger
                };
                ret = sensor_trigger_set(mpu,&tri,NULL);
                if (ret)
                {
                    printk("error: can not set triger for mpu 6500, code %d\n",ret);
                    return ret;
                }
                flag = 0;
                break;
            }
            k_msleep(50);
        }
  ~~~~~
 - 需要注意的是，MPU6500在设置完触发后，会拉高触发引脚`5ms`左右，这个拉高的动作同样会进入一次中断，所以在触发之后，建议设置一个多次触发的阈值，确实多次重复触发再进行下一步的动作，不然会出现无故一直触发的情况，或者直接将第一次触发跳过。