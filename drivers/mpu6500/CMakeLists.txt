#  declares the current directory as z zephyr linary

zephyr_library()

# list thr sourece file
zephyr_library_sources(mpu6500.c)
zephyr_library_sources_ifdef(CONFIG_MPU6500_TRIGGER mpu6500_trigger.c)

# get include dir
zephyr_include_directories(.)
