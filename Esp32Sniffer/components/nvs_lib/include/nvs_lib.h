#ifndef A62773E8_A184_46FE_BA12_A9DF240BEDCF
#define A62773E8_A184_46FE_BA12_A9DF240BEDCF

#include "nvs.h"
esp_err_t nvs_set_num_from_string(const char *namespace, const char *key, nvs_type_t type, const char *str_value);
esp_err_t nvs_set_num32i(const char *namespace, const char *key, const int32_t value);
esp_err_t nvs_get_num32i(const char *namespace, const char *key, int32_t *value, const int32_t def_value);
esp_err_t nvs_get_num(const char *namespace, const char *key, nvs_type_t type, void *out_value, const void *def_value);
esp_err_t nvs_set_string(const char *namespace, const char *key, const char *str_value);
esp_err_t nvs_get_string(const char *namespace, const char *key, char *out_value, const char *def_value, size_t def_value_sz);
esp_err_t nvs_erase_value(const char *namespace, const char *key);
void nvs_init_flash();
#endif /* A62773E8_A184_46FE_BA12_A9DF240BEDCF */
