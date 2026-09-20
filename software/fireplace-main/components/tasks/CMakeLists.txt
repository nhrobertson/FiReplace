idf_component_register(SRCS "tasks.c"
                    INCLUDE_DIRS "include"
                    REQUIRES link config io esp_timer freertos
)
