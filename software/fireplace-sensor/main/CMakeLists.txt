idf_component_register(SRCS "fireplace-sensor.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES i2cdev sht4x link
)
