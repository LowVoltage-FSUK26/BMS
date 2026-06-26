/*
 * Temperatures.h
 *
 *  Created on: Apr 12, 2026
 *      Author: AbdUllah
 */

#ifndef INC_TEMPERATURES_H_
#define INC_TEMPERATURES_H_
#define NUM_GPIOS   4  // change this to match active thermistors per slave

#define NTC_TABLE_SIZE  (sizeof(ntc_table) / sizeof(ntc_table[0]))
#define R_PULLUP      10000.0f
// Lookup table from datasheet — CENTER resistance values
typedef struct {
    int16_t  temp_C;      // temperature in °C
    uint32_t resistance;  // center resistance in Ω
} NTC_Point;


extern uint16_t GpioReadings[SLAVEBOARDS][NUM_GPIOS];
extern uint16_t TsrefReadings[SLAVEBOARDS];
extern float temperatures[SLAVEBOARDS][NUM_GPIOS];
uint8_t configure_OTUT(uint8_t dev_address, uint8_t activeThermistors);
uint8_t configureGPIO(uint8_t GPIO_NUM, BQ79616_GPIO_Config_t GPIOMODE ,uint8_t BID, uint8_t bWriteType);
uint16_t readGPIOVoltage(uint8_t BID, uint8_t GPIO_NUM, uint16_t* raw_value_ptr);
void readAllGPIO(void);
void readTSREF(void);
void convertAllTemperatures(void);
float convertNTCtoTemp(uint16_t raw_adc,uint16_t raw_TSREF);
#endif /* INC_TEMPERATURES_H_ */
