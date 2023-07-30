#include <stdio.h>
#include "../../build/config/sdkconfig.h"
#include "OledDisplayUtils.h"

#include <ssd1306.h>

#define I2C_MASTER_NUM CONFIG_I2C_MASTER_PORT /*!< I2C port number for master dev */
static ssd1306_handle_t ssd1306_dev = NULL;
void initDisplay()
{

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)CONFIG_SDA_GPIO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)CONFIG_SCL_GPIO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = CONFIG_I2C_MASTER_CLK_FREQ; 
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, SSD1306_I2C_ADDRESS);
    ssd1306_refresh_gram(ssd1306_dev);
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    //return ssd1306_dev;
}

void displayPrint(uint8_t chXpos, uint8_t chYpos, const char *fmt, ...)
{

    char buffer[50] = "";
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    va_end(argptr);

    ssd1306_draw_string(ssd1306_dev, chXpos, chYpos, (const uint8_t *)buffer, 16, 1);    
    ssd1306_refresh_gram(ssd1306_dev);
}