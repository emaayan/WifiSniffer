#ifndef E14E7AD1_D11D_4C3D_B5F6_BD715252D110
#define E14E7AD1_D11D_4C3D_B5F6_BD715252D110

#define LGFX_USE_V1
#include "../../build/config/sdkconfig.h"

#if  CONFIG_IDF_TARGET_ESP32S3
#include <LovyanGFX.h>
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void);
};
#endif

#endif /* E14E7AD1_D11D_4C3D_B5F6_BD715252D110 */
