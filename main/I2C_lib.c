#include "I2C_lib.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/i2c.h"

/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
uint8_t reg_read(uint8_t slv_address, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_master_write_read_device(0, slv_address, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
    return 0;
}

/**
 * @brief Write a byte to a MPU9250 sensor register
 */
uint8_t reg_write(uint8_t slv_address, uint8_t reg_addr, const uint8_t *data, uint8_t data_len)
{
    int ret;
    uint8_t *write_buf = (uint8_t*) malloc(sizeof(uint8_t) * (data_len + 1));
    write_buf[0] = reg_addr;
    for (int i = 1; i < data_len + 1; i++) {
        write_buf[i] = data[i - 1];
    }

    i2c_master_write_to_device(0, slv_address, write_buf, sizeof(write_buf), 1000 / portTICK_PERIOD_MS);
    free(write_buf);

    return 0;
}
uint8_t i2c_write(uint8_t slv_address, const uint8_t *data, uint8_t data_len)
{
    i2c_master_write_to_device(0, slv_address, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
    return 0;
}

