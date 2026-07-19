idf_component_register(SRCS "fireplace-remote.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_timer ssd1306 link io
)
