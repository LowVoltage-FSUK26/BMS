/*
 * BMS_tests.c
 *
 *  Created on: Jan 28, 2026
 *      Author: Mina Fathy
 */

#include "main.h"
#include "BMS_tests.h"
#include "bq79616.h"
#include "bq79600.h"


extern int totalV;
extern float final_value;
extern int cellVoltages_board0[16];
extern int cellVoltages_board1[16];
extern UART_HandleTypeDef huart2;

void test1(){ //success!!!
	Wake79616();

	HAL_Delay(2);

	AutoAddress();

}


/** Test Case 2: Voltage Reading **/

float test2(){

	int slave_totalV[SLAVEBOARDS] = {0};
	int totalV = 0;

	uint8_t activeCells = 16;

	for(uint8_t i = 1; i <= SLAVEBOARDS; i++ )
	{
		readBoardVoltages(i, activeCells, (uint8_t*)&slave_totalV[i], cellVoltages_board0);
		totalV += slave_totalV[i];
		vTaskDelay(100);		//Check for the minimum delay that can be achieved
	}

	return (totalV * 0.00019073);

}


/** Test Case 3: Configurations Needed for fault detection **/
void test3(){ //success!!!

	//enable temp protector using 1 gpio thermistor
	//remember to set vcb done to vuv
	uint8_t status_OTUT = configure_OTUT(0, 1);

	HAL_UART_Transmit(&huart2, &status_OTUT, sizeof(status_OTUT), 100);
	HAL_Delay(50);

	//enable voltage protector for board 0 all 16 cells
	uint8_t status_OVUV = configure_OVUV(0, 16);

	HAL_UART_Transmit(&huart2, &status_OVUV, sizeof(status_OVUV), 100);

	/** expected: undervoltage fault **/
	//further testing fault interrupt test
}


uint16_t test4()
{
	uint16_t comm_x = 0;
	uint8_t fault_array[TOTALBOARDS] = {0};
	uint8_t msg_received = 0;
	uint8_t read_reg = 0;
	Wake79616();

	//HAL_Delay(2);

	AutoAddress_Ring();

	//Time for physically removing the cable between 2 EVMs
	//    HAL_Delay(3000);

	//Reset all faults
	ResetAllFaults(0, FRMWRT_ALL_W);

	//Read for communication Error
	for(int i = 0; i < TOTALBOARDS; i++)
	{
		if(readReg(i, FAULT_RST2, fault_array + i, 1, 200, FRMWRT_SGL_R) !=0)
		{
			msg_received++;
		}
		fault_array[i] = fault_array[i] & 0x1F;

		if(fault_array[i] != 0)
			break;
	}

	//Communication error
	if(msg_received != TOTALBOARDS)
	{
		//Print using uart "Communication Fault detected"
		//Reverse Communication Direction of Base
		readReg(0, BQ79616_CONTROL1, &read_reg, 1, 200, FRMWRT_SGL_R);
		writeReg(0, BQ79616_CONTROL1, read_reg | (1 << 7), 1, FRMWRT_SGL_W);

		//Revese Communication Direction for the reset of chain
		for(int i = 0; i < TOTALBOARDS; i++)
		{
			readReg(i, BQ79616_CONTROL1, &read_reg, 1, 200, FRMWRT_SGL_R);
			writeReg(i, BQ79616_CONTROL1, (1 << 7), 1, FRMWRT_SGL_W);
		}
	}
	else
	{
		//Print using uart "NO commuincation fault"
		comm_x = 0;
	}

	//repeat test again after reversing direction
	if(readReg(TOTALBOARDS - msg_received, BQ79616_CONTROL1, &read_reg, 1, 200, FRMWRT_SGL_R) != 0)
	{
		//print via uart read_reg
		comm_x = 1;
	}
	else
	{
		//print via uart "Coundn't reach board"
		comm_x = 200;
	}
	return comm_x;
}


