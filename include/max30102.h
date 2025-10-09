#ifndef MAX30102_H
#define MAX30102_H
#include "esp_event.h"
#include <esp_log.h>
/**
 * @brief chip register definition
 */
#define MAX30102_REG_INTERRUPT_STATUS_1          0x00        /**< interrupt status 1 register */
#define MAX30102_REG_INTERRUPT_STATUS_2          0x01        /**< interrupt status 2 register */
#define MAX30102_REG_INTERRUPT_ENABLE_1          0x02        /**< interrupt enable 1 register */
#define MAX30102_REG_INTERRUPT_ENABLE_2          0x03        /**< interrupt enable 2 register */
#define MAX30102_REG_FIFO_WRITE_POINTER          0x04        /**< fifo write pointer register */
#define MAX30102_REG_OVERFLOW_COUNTER            0x05        /**< overflow counter register */
#define MAX30102_REG_FIFO_READ_POINTER           0x06        /**< fifo read pointer register */
#define MAX30102_REG_FIFO_DATA_REGISTER          0x07        /**< fifo data register */
#define MAX30102_REG_FIFO_CONFIG                 0x08        /**< fifo config register */
#define MAX30102_REG_MODE_CONFIG                 0x09        /**< mode config register */
#define MAX30102_REG_SPO2_CONFIG                 0x0A        /**< spo2 config register */
#define MAX30102_REG_LED_PULSE_1                 0x0C        /**< led pulse amplitude 1 register */
#define MAX30102_REG_LED_PULSE_2                 0x0D        /**< led pulse amplitude 2 register */
#define MAX30102_REG_MULTI_LED_MODE_CONTROL_1    0x11        /**< multi led mode control 1 register */
#define MAX30102_REG_MULTI_LED_MODE_CONTROL_2    0x12        /**< multi led mode control 2 register */
#define MAX30102_REG_DIE_TEMP_INTEGER            0x1F        /**< die temperature integer register */
#define MAX30102_REG_DIE_TEMP_FRACTION           0x20        /**< die temperature fraction register */
#define MAX30102_REG_DIE_TEMP_CONFIG             0x21        /**< die temperature config register */
#define MAX30102_REG_REVISION_ID                 0xFE        /**< revision id register */
#define MAX30102_REG_PART_ID                     0xFF        /**< part id register */

typedef enum            //@sample_average
{
    sample_average_1 = 0,
    sample_average_2 = 1,
    sample_average_4 = 2,
    sample_average_8 = 3,
    sample_average_16 = 4,
    sample_average_32 = 5
}sample_avg ;

typedef enum 
{
    Heart_rate_mode = 2,   //red
    Spo2_mode = 3,         //IR
    Multi_LED_mode =7
}mode_select;

typedef enum 
{
    ADC_RESOLUTION_15 = 0,
    ADC_RESOLUTION_16 = 1,
    ADC_RESOLUTION_17 = 2,
    ADC_RESOLUTION_18 =3 
    
}LED_PW;

typedef enum  {
	MAX30102_SLOT_DISABLED		= 0,
	MAX30102_SLOT_RED_LED1_PA,
	MAX30102_SLOT_IR_LED2_PA,
}max30102_slot;

typedef enum 
{
    SENSOR_CHAN_RED = 0,
    SENSOR_CHAN_IR
}sensor_channel;

typedef enum  {
	MAX30101_LED_CHANNEL_RED	= 0,
	MAX30101_LED_CHANNEL_IR,
}max30102_led_channel;


#define MAX30102_SLOT_LED_MASK		0x3

#define MAX30102_BYTES_PER_CHANNEL	3
#define MAX30102_MAX_NUM_CHANNELS	2
#define MAX30102_MAX_BYTES_PER_SAMPLE	(MAX30102_MAX_NUM_CHANNELS * \
					 MAX30102_BYTES_PER_CHANNEL)


#define MAX30102_FIFO_DATA_BITS		18        //As data is in 18 bits not in 24 bits
#define MAX30102_FIFO_DATA_MASK		((1 << MAX30102_FIFO_DATA_BITS) - 1)   //create mask 0x3FFFF for 18bits 

typedef struct max_data
{
    uint32_t raw[MAX30102_MAX_NUM_CHANNELS];      //stores the 18-bit ADC value
	uint8_t map[MAX30102_MAX_NUM_CHANNELS];       //map led channel to fifo channel
	uint8_t num_channels;                         //current no.of channels
}max30102_data;

void verify_device();
void sensor_init( max30102_data *data, mode_select mode, uint8_t sample_average);
esp_err_t sample_fetch_max30102(max30102_data *data);
esp_err_t get_channel(sensor_channel channel, max30102_data *data, uint32_t* val);

#endif