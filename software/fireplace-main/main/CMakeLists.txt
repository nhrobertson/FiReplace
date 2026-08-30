idf_component_register(SRCS "fireplace-main.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash link config tasks
)
