#include <stdio.h>
#include "esp_app_trace.h"
#include "esp_log.h"


esp_err_t reg_read(uint8_t slv_address, uint8_t reg_addr, uint8_t *data, size_t len);

esp_err_t reg_write(uint8_t slv_address, uint8_t reg_addr, const uint8_t *data, uint8_t data_len);

esp_err_t i2c_write(uint8_t slv_address, const uint8_t *data, uint8_t data_len);
