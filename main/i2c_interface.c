#include "i2c_interface.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include <esp_log.h>

#define I2C_MASTER_SDA_IO  5
#define I2C_MASTER_SCL_IO  6
#define I2C_SLAVE_NUM    I2C_NUM_0
#define TEST_I2C_PORT    I2C_SLAVE_NUM
#define DEVICE_ADDR        0x57

const char* TAG_i2c = "I2C_BUS";

i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = TEST_I2C_PORT,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = DEVICE_ADDR,    
    .scl_speed_hz = 100000,
};

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

void i2c_init(void)
{   esp_err_t ret;

    ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_i2c,"Failed to initialize I2C bus");
    }
    
    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_i2c,"I2C master device initialization failed");
    }
    
}


esp_err_t ReadRegister(uint8_t reg_addr, uint8_t read_size, uint8_t *data)
{
   return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, read_size , pdMS_TO_TICKS(1000));
}

esp_err_t WriteRegister(uint8_t reg_addr, uint8_t data)
{   
    uint8_t write_buff[2]={reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buff, sizeof(write_buff), pdMS_TO_TICKS(1000));
}