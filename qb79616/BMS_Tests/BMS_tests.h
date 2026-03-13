/*
 * BMS_tests.h
 *
 *  Created on: Jan 28, 2026
 *      Author: Mina Fathy
 */

#ifndef INC_BMS_TESTS_H_
#define INC_BMS_TESTS_H_

#include "BMS_Config.h"

//============================
//Test Functions Prototypes
//============================

void test1(void); 						//Initialization Test
float readBattaryVoltage(void); 		//Cell Voltage readings Test, prev. named test2
void test3(void);						//Configurations Needed for fault detection
uint16_t test4(void);					//ERROR Diagnostic report request and Ring topology

BMS_StatusTypeDef readSlaveVoltage(uint8_t salve_num, uint8_t activeCells, int* raw_value);

#endif /* INC_BMS_TESTS_H_ */
