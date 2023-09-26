idf_component_register(SRCS "webserver.c" "alarm_clock.c" "main.c" "light_strip.c" "DS1307.c" "I2C_lib.c" "lcd_display.c"
EMBED_TXTFILES web/index.html

                    INCLUDE_DIRS ".")
