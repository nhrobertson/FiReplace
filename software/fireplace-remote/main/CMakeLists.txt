idf_component_register(SRCS "fireplace-remote.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_wifi esp_timer ssd1306 link io
)
