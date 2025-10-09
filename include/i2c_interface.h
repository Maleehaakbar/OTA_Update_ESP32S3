#ifndef I2C_INTERFACE_H
#define I2C_INTERFACE_H
#include "driver/i2c_master.h"

void i2c_init(void);
esp_err_t ReadRegister(uint8_t reg_addr, uint8_t read_size, uint8_t *data);
esp_err_t WriteRegister(uint8_t reg_addr, uint8_t data);

#endif