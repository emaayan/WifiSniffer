set(REQ, "esp_app_format"                    
                    "console" 
                    "tcp_server" 
                    "wifi_sniffer"                                         
                    "led_lib"
                    )
                    
if(CONFIG_USE_OLED)
	list(APPEND REQ oled_display_lib)
endif()

if(CONFIG_USE_LCD)
	list(APPEND REQ lcd_lib)
endif()

                    
idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES ${REQ} )                    

#idf_component_register(
#    SRCS tmpmain.c         # list the source files of this component
#    INCLUDE_DIRS        # optional, add here public include directories
#    REQUIRES            # optional, list the public requirements (component names)
#)
