/*
 * Temperatures.h
 *
 *  Created on: Apr 12, 2026
 *      Author: AbdUllah
 */

#ifndef INC_TEMPERATURES_H_
#define INC_TEMPERATURES_H_
extern uint16_t GpioReadings[SLAVEBOARDS][4];
uint8_t configure_OTUT(uint8_t dev_address, uint8_t activeThermistors);
uint8_t configureGPIO(uint8_t GPIO_NUM, BQ79616_GPIO_Config_t GPIOMODE ,uint8_t BID, uint8_t bWriteType);
uint16_t readGPIOVoltage(uint8_t BID, uint8_t GPIO_NUM, uint16_t* raw_value_ptr);

#endif /* INC_TEMPERATURES_H_ */
