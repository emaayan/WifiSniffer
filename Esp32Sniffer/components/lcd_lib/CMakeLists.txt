#if(CONFIG_ENABLE_LCD_OUTPUT)   
    #set(SRCS "lcd_lib.c" "display_lib.cpp" "LGFX_DisplayS3.cpp")
#else()
 #   set(SRCS "lcd_lib.c" "display_lib.c")   
#endif()

idf_build_get_property(target IDF_TARGET)

if(target STREQUAL "esp32s3")
set(SRCS "lcd_lib.c" "display_lib.cpp" "LGFX_DisplayS3.cpp")
    idf_component_register(SRCS "${SRCS}"
                        INCLUDE_DIRS "include" 
                        REQUIRES LovyanGFX 
                        )
else()
    set(SRCS "lcd_lib.c" "display_lib.c")   
    idf_component_register(SRCS "${SRCS}"
                        INCLUDE_DIRS "include" 
                        )
endif()


