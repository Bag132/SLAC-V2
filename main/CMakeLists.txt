idf_component_register(SRCS 
                        "src/webserver.c" 
                        "src/alarm_clock.c" 
                        "main.c"
                        "src/light_strip.c" 
                        "src/DS1307.c" 
                        "src/I2C_lib.c" 
                        "src/lcd_display.c" 
                        EMBED_TXTFILES web/index.html
                        INCLUDE_DIRS "." "inc" "src")
