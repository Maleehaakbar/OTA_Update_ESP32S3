#include "max30102.h"
#include "i2c_interface.h"

#define DEVICE_ID       0x15
const char* TAG = "MAX_SENSOR";

void FIFO_config(uint8_t sample_average);
void Mode_ctrl(uint8_t mode);
void SPO2_config(uint8_t ADC_range, uint8_t SPO2_sampling, uint8_t LED_PW);

void verify_device(void)
{   uint8_t part_ID;
    esp_err_t ret;

    ret = ReadRegister((uint8_t)MAX30102_REG_PART_ID, 1,&part_ID);
    if (ret == ESP_OK)
    {
       ESP_LOGI(TAG,"Device ID is %x", part_ID);
       if (part_ID != DEVICE_ID)
       {
            ESP_LOGE(TAG,"Device ID not match expected ID");
       }
    }
    else
    {
        ESP_LOGE(TAG,"Failed to read device ID");
    }
    
}

void sensor_init( max30102_data *data, mode_select mode, uint8_t sample_average )
{   esp_err_t ret;
    uint8_t mode_reg;
    uint8_t led_current =0x1F;
    uint32_t led_chan;
    int fifo_chan;
    max30102_slot slot [4];

    verify_device();
    ret = WriteRegister(MAX30102_REG_MODE_CONFIG,1<<6);           //reset
    if (ret!=ESP_OK)
    {
        ESP_LOGE(TAG,"Failed to reset sensor");
        return;
    }

    do {
        ret = ReadRegister(MAX30102_REG_MODE_CONFIG,1,&mode_reg);
        if (ret!=ESP_OK)
        {
            ESP_LOGE(TAG,"Couldn't clear reset");
            return;
        }

    }while ((mode_reg & (1<<6)) !=0);

    WriteRegister(MAX30102_REG_FIFO_CONFIG ,1<<4 |sample_average<<5); 
    WriteRegister(MAX30102_REG_MODE_CONFIG, mode<<0);
    SPO2_config(0,0, ADC_RESOLUTION_15);
    WriteRegister(MAX30102_REG_LED_PULSE_1,led_current << 0);
    WriteRegister(MAX30102_REG_LED_PULSE_2,led_current << 0);

    if (mode == Heart_rate_mode)
    {
        slot[0] = MAX30102_SLOT_RED_LED1_PA;
        slot[1] = MAX30102_SLOT_DISABLED;
        slot[2] = MAX30102_SLOT_DISABLED;
        slot[3] = MAX30102_SLOT_DISABLED;
    }

    else if (mode == Spo2_mode)
    {
        slot[0] = MAX30102_SLOT_RED_LED1_PA;
        slot[1] = MAX30102_SLOT_IR_LED2_PA;
        slot[2] = MAX30102_SLOT_DISABLED;
        slot[3] = MAX30102_SLOT_DISABLED;
    }
 
    data->num_channels = 0U;
	for (led_chan = 0U; led_chan < MAX30102_MAX_NUM_CHANNELS; led_chan++) {
		data->map[led_chan] = MAX30102_MAX_NUM_CHANNELS;
	}
    
    for (fifo_chan =0; fifo_chan < MAX30102_MAX_NUM_CHANNELS; fifo_chan++)
    { 
        led_chan = (slot[fifo_chan] & MAX30102_SLOT_LED_MASK)-1;  //convert 1-based LED ID to zeroth based array index
        if (led_chan < MAX30102_MAX_NUM_CHANNELS) {
			data->map[led_chan] = fifo_chan;
			data->num_channels++;
            ESP_LOGI(TAG,"value of num_channels, led_chan , fifo_chan %u %lu %d ", data->num_channels, led_chan, fifo_chan);

		}
    }

}

void SPO2_config(uint8_t ADC_range, uint8_t SPO2_sampling, uint8_t LED_PW)
{
    esp_err_t ret;
    ret = WriteRegister(MAX30102_REG_SPO2_CONFIG,LED_PW <<0| SPO2_sampling <<2| ADC_range <<5 );
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG,"SPO2 config error");
        return;
    }

}

esp_err_t get_channel(sensor_channel channel, max30102_data *data, uint32_t* val)
{   int fifo_chan;
    max30102_led_channel led_chan;
    switch(channel)
    {
        case(SENSOR_CHAN_RED):
            led_chan = MAX30101_LED_CHANNEL_RED;
            break;
        case(SENSOR_CHAN_IR):
            led_chan = MAX30101_LED_CHANNEL_IR;
            break;
        default:
            ESP_LOGE(TAG,"Unsupported sensor channel");
            return ESP_FAIL;

    }
    fifo_chan = data->map[led_chan];
	if (fifo_chan >= MAX30102_MAX_NUM_CHANNELS) {
		ESP_LOGE(TAG,"Inactive sensor channel");
		return ESP_FAIL;
	}

    *val = data->raw[fifo_chan];

   return ESP_OK;
}

esp_err_t sample_fetch_max30102(max30102_data *data)
{  
    esp_err_t ret;
    uint8_t buff[MAX30102_MAX_BYTES_PER_SAMPLE];
    uint32_t fifo_data;
	int fifo_chan;
	uint8_t num_bytes;
	int i;
    num_bytes = data-> num_channels * MAX30102_BYTES_PER_CHANNEL;
    ESP_LOGI(TAG, "number of bytes are %u", num_bytes);
    ret = ReadRegister(MAX30102_REG_FIFO_DATA_REGISTER , num_bytes, buff);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG,"Couldnt read FIFO data");
        return ESP_FAIL;
    }
    
    fifo_chan = 0;
    for (i =0; i<num_bytes; i+=3)
    {
        fifo_data = (buff[i] << 16) | (buff[i + 1] << 8) |
			    (buff[i + 2]);
		fifo_data &= MAX30102_FIFO_DATA_MASK;             
        data->raw[fifo_chan++] = fifo_data;        //1st 3 bytes (data->raw[0]) RED channel , second three bytes IR channel.
                                                    //  In get channel , get the value of selected channel
    }
    
    return ESP_OK;
}
