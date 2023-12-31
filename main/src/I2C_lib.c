#include "I2C_lib.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/i2c.h"
#include <pthread.h>

static pthread_mutex_t i2c_lock;

/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
esp_err_t reg_read(uint8_t slv_address, uint8_t reg_addr, uint8_t *data, uint8_t len)
{

    return i2c_master_write_read_device(0, slv_address, &reg_addr, 1, data, len * sizeof(uint8_t), 1000 / portTICK_PERIOD_MS);
}

/**
 * @brief Write a byte to a MPU9250 sensor register
 */
esp_err_t reg_write(uint8_t slv_address, uint8_t reg_addr, const uint8_t *data, uint8_t data_len)
{
    uint8_t buf_len = data_len + 1;
    uint8_t *write_buf = (uint8_t*) malloc(sizeof(uint8_t) * buf_len);
    write_buf[0] = reg_addr;
    for (int i = 1; i < buf_len; i++) {
        write_buf[i] = data[i - 1];
    }

    esp_err_t ret = i2c_master_write_to_device(0, slv_address, write_buf, buf_len * sizeof(uint8_t), 1000 / portTICK_PERIOD_MS);
    free(write_buf);

    return ret;
}

esp_err_t i2c_write(uint8_t slv_address, const uint8_t *data, uint8_t data_len)
{
    return i2c_master_write_to_device(0, slv_address, data, data_len * sizeof(uint8_t), 1000 / portTICK_PERIOD_MS);
}

esp_err_t i2c_read(uint8_t slv_address, uint8_t *data, uint8_t data_len)
{
    return i2c_master_read_from_device(0, slv_address, data, data_len * sizeof(uint8_t), 1000 / portTICK_PERIOD_MS);
}
