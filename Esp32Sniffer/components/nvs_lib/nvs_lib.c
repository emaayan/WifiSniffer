#include "nvs_lib.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"

#include "esp_err.h"
#include <errno.h>

#include "nvs_flash.h"

static const char *TAG = "nvs_lib";

esp_err_t nvs_erase_value(const char *namespace, const char *key)
{
    nvs_handle_t nvs;

    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err == ESP_OK)
    {
        err = nvs_erase_key(nvs, key);
        if (err == ESP_OK)
        {
            err = nvs_commit(nvs);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "Value with key '%s' erased", key);
            }
        }
        nvs_close(nvs);
    }

    return err;
}

static esp_err_t nvs_erase_all_values(const char *namespace)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err == ESP_OK)
    {
        err = nvs_erase_all(nvs);
        if (err == ESP_OK)
        {
            err = nvs_commit(nvs);
        }
    }

    ESP_LOGI(TAG, "Namespace '%s' was %s erased", namespace, (err == ESP_OK) ? "" : "not");

    nvs_close(nvs);
    return ESP_OK;
}

int nvs_list_values(const char *partition, const char *namespace, nvs_type_t type)
{

    nvs_iterator_t it = NULL;
    esp_err_t result = nvs_entry_find(partition, namespace, type, &it);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGE(TAG, "No such entry was found in partition %s ", partition);
        return 1;
    }

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS error: %s", esp_err_to_name(result));
        return 1;
    }

    do
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        result = nvs_entry_next(&it);

        // printf("namespace '%s', key '%s', type '%s' \n", info.namespace_name, info.key, type_to_str(info.type));
    } while (result == ESP_OK);

    if (result != ESP_ERR_NVS_NOT_FOUND)
    { // the last iteration ran into an internal error
        ESP_LOGE(TAG, "NVS error %s at current iteration, stopping.", esp_err_to_name(result));
        return 1;
    }

    return 0;
}

esp_err_t nvs_set_num32i(const char *namespace, const char *key, const int32_t value)
{
    esp_err_t err;
    nvs_handle_t nvs;
    err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        return err;
    }
    err = nvs_set_i32(nvs, key, value);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Value stored under key '%s'", key);
        }
    }

    nvs_close(nvs);
    return err;
}

esp_err_t nvs_get_num32i(const char *namespace, const char *key, int32_t *out_value, const int32_t def_value)
{
    esp_err_t err;
    nvs_handle_t nvs;
    err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        return err;
    }
    int32_t value;
    if ((err = nvs_get_i32(nvs, key, &value)) == ESP_OK)
    {
        *out_value = value;
    }
    else
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            *out_value = def_value;
            err = ESP_OK;
        }
    }

    nvs_close(nvs);
    return err;
}
esp_err_t nvs_set_num_from_string(const char *namespace, const char *key, nvs_type_t type, const char *str_value)
{
    esp_err_t err;
    nvs_handle_t nvs;
    err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        return err;
    }

    bool range_error = false;
    switch (type)
    {
    case NVS_TYPE_I8:
    {
        int32_t value = strtol(str_value, NULL, 0);
        if (value < INT8_MIN || value > INT8_MAX || errno == ERANGE)
        {
            range_error = true;
        }
        else
        {
            err = nvs_set_i8(nvs, key, (int8_t)value);
        }
    }
    break;
    case NVS_TYPE_U8:
    {
        uint32_t value = strtoul(str_value, NULL, 0);
        if (value > UINT8_MAX || errno == ERANGE)
        {
            range_error = true;
        }
        else
        {
            err = nvs_set_u8(nvs, key, (uint8_t)value);
        }
    }
    break;
    case NVS_TYPE_I16:
    {
        int32_t value = strtol(str_value, NULL, 0);
        if (value < INT16_MIN || value > INT16_MAX || errno == ERANGE)
        {
            range_error = true;
        }
        else
        {
            err = nvs_set_i16(nvs, key, (int16_t)value);
        }
    }
    break;
    case NVS_TYPE_U16:
    {
        uint32_t value = strtoul(str_value, NULL, 0);
        if (value > UINT16_MAX || errno == ERANGE)
        {
            range_error = true;
        }
        else
        {
            err = nvs_set_u16(nvs, key, (uint16_t)value);
        }
    }
    break;
    case NVS_TYPE_I32:
    {
        int32_t value = strtol(str_value, NULL, 0);
        if (errno != ERANGE)
        {
            err = nvs_set_i32(nvs, key, value);
        }
    }
    break;
    case NVS_TYPE_U32:
    {
        uint32_t value = strtoul(str_value, NULL, 0);
        if (errno != ERANGE)
        {
            err = nvs_set_u32(nvs, key, value);
        }
    }
    break;
    case NVS_TYPE_I64:
    {
        int64_t value = strtoll(str_value, NULL, 0);
        if (errno != ERANGE)
        {
            err = nvs_set_i64(nvs, key, value);
        }
    }
    break;
    case NVS_TYPE_U64:
    {
        uint64_t value = strtoull(str_value, NULL, 0);
        if (errno != ERANGE)
        {
            err = nvs_set_u64(nvs, key, value);
        }
    }
    break;
    default:
        ESP_LOGE(TAG, "Type  is not allowed");
        return ESP_ERR_NVS_TYPE_MISMATCH;
    }

    if (range_error || errno == ERANGE)
    {
        nvs_close(nvs);
        return ESP_ERR_NVS_VALUE_TOO_LONG;
    }

    if (err == ESP_OK)
    {
        err = nvs_commit(nvs);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Value stored under key '%s'", key);
        }
    }

    nvs_close(nvs);
    return err;
}

esp_err_t nvs_get_num(const char *namespace, const char *key, nvs_type_t type, void *out_value, const void *def_value)
{

    esp_err_t err;

    nvs_handle_t nvs;
    err = nvs_open(namespace, NVS_READONLY, &nvs);

    if (err != ESP_OK)
    {
        return err;
    }

    switch (type)
    {
    case NVS_TYPE_I8:
    {
        int8_t value;
        err = nvs_get_i8(nvs, key, &value);
        if (err == ESP_OK)
        {
            *(int8_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_U8:
    {
        uint8_t value;
        err = nvs_get_u8(nvs, key, &value);
        if (err == ESP_OK)
        {
            *(uint8_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_I16:
    {
        int16_t value;
        err = nvs_get_i16(nvs, key, &value);
        if (err == ESP_OK)
        {
            *(int16_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_U16:
    {
        uint16_t value;
        if ((err = nvs_get_u16(nvs, key, &value)) == ESP_OK)
        {
            *(uint16_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_I32:
    {
        int32_t value;
        if ((err = nvs_get_i32(nvs, key, &value)) == ESP_OK)
        {
            *(int32_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_U32:
    {
        uint32_t value;
        if ((err = nvs_get_u32(nvs, key, &value)) == ESP_OK)
        {
            *(uint32_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_I64:
    {
        int64_t value;
        if ((err = nvs_get_i64(nvs, key, &value)) == ESP_OK)
        {
            *(int64_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_U64:
    {
        uint64_t value;
        if ((err = nvs_get_u64(nvs, key, &value)) == ESP_OK)
        {
            // printf("%llu\n", value);
            *(uint64_t *)out_value = value;
        }
    }
    break;
    case NVS_TYPE_STR:
    {
        size_t len;
        if ((err = nvs_get_str(nvs, key, NULL, &len)) == ESP_OK)
        {
            if ((err = nvs_get_str(nvs, key, out_value, &len)) == ESP_OK)
            {
                // printf("%s\n", str);
            }
        }
        else
        {
            if (err == ESP_ERR_NVS_NOT_FOUND)
            {
                memcpy(out_value, def_value, len);
                err = ESP_OK;
                // out_value=def_value;
            }
        }
    }
    break;
    default:
        ESP_LOGE(TAG, "Type is undefined");
        return ESP_ERR_NVS_TYPE_MISMATCH;
    }

    nvs_close(nvs);
    return err;
}

esp_err_t nvs_set_string(const char *namespace, const char *key, const char *str_value)
{
    esp_err_t err;

    nvs_handle_t nvs;
    err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(nvs, key, str_value);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Value stored under key '%s'", key);
        }
    }

    nvs_close(nvs);
    return err;
}

esp_err_t nvs_get_string(const char *namespace, const char *key, char *out_value, const char *def_value, size_t def_value_sz)
{
    esp_err_t err;

    nvs_handle_t nvs;
    err = nvs_open(namespace, NVS_READONLY, &nvs);

    if (err != ESP_OK)
    {
        return err;
    }

    size_t len;
    if ((err = nvs_get_str(nvs, key, NULL, &len)) == ESP_OK)
    {
        if ((err = nvs_get_str(nvs, key, out_value, &len)) != ESP_OK)
        {            
            ESP_LOGE(TAG, "Error in Getting String value");
        }
        else
        {
            ESP_LOGI(TAG, "key %s is found in namespace %s, using value %s ", key, namespace, out_value);
        }
    }
    else
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(TAG, "key %s is not found in namespace %s using default %s ", key, namespace,def_value);
            memcpy(out_value, def_value, def_value_sz);
            err = ESP_OK;
        }
    }

    nvs_close(nvs);
    return err;
}

void nvs_init_flash()
{
    ESP_LOGI(TAG, "Init NVS Flash");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "Problem init flash %d, erasing flash", ret);
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}