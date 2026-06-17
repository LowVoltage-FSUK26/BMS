/*
 * GUI.c
 *
 *  Created on: Mar 13, 2026
 *      Author: AbdUllah
 */
#include <stdint.h>
#include "BMS_Config.h"
#include "GUI.h"
#include "Temperatures.h"
#include "Faults.h"
#include "Voltages.h"
#include "Temperatures.h"

extern UART_HandleTypeDef huart2; //huart2 is used only for debugging


static char HEX_DIGITS[] = "0123456789ABCDEF";
void uint16_to_hex4( char *out,uint16_t value)
{
	out[0] = HEX_DIGITS[(value >> 12) & 0xF];
	out[1] = HEX_DIGITS[(value >> 8)  & 0xF];
	out[2] = HEX_DIGITS[(value >> 4)  & 0xF];
	out[3] = HEX_DIGITS[value & 0xF];
}

void uint8_to_hex2( char *out,uint8_t value)
{
	out[0] = HEX_DIGITS[(value >> 4) & 0xF];
	out[1] = HEX_DIGITS[value & 0xF];
}

void UART_Send_Board_Frame_CharByChar(uint8_t board_num)
{
	char temp[5];
	// ----- S + board number -----
	HAL_UART_Transmit(&huart2, (uint8_t*)"S", 1, HAL_MAX_DELAY);
	char board_char = '0' + (board_num); // S1, S2 ...
	HAL_UART_Transmit(&huart2, (uint8_t*)&board_char, 1, HAL_MAX_DELAY);

	// ----- Separator -----
	HAL_UART_Transmit(&huart2, (uint8_t*)"|", 1, HAL_MAX_DELAY);
	if(board_num==0)
	{
		// ----- Fault -----
		uint8_to_hex2(temp,bridge_faultSummary );

		for(int j=0;j<2;j++)
			HAL_UART_Transmit(&huart2, (uint8_t*)&temp[j], 1, HAL_MAX_DELAY);

		// ----- End of Frame -----
		HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
	}
	else {
		// ----- Cells -----
		cellVoltages_board[0][15]=0;
		for(int i=0;i<16;i++)
		{
			uint16_to_hex4(temp, cellVoltages_board[board_num-1][i]); // 4-digit hex
			for(int j=0;j<4;j++)
				HAL_UART_Transmit(&huart2, (uint8_t*)&temp[j], 1, HAL_MAX_DELAY);

			if(i != 16-1)
				HAL_UART_Transmit(&huart2, (uint8_t*)",", 1, HAL_MAX_DELAY);
		}

		// ----- Separator -----
		HAL_UART_Transmit(&huart2, (uint8_t*)"|", 1, HAL_MAX_DELAY);

		// ----- GPIOs -----
		for(int i=0;i<4;i++)
		{
			uint16_to_hex4(temp,GpioReadings[board_num-1][i]);
			for(int j=0;j<4;j++)
				HAL_UART_Transmit(&huart2, (uint8_t*)&temp[j], 1, HAL_MAX_DELAY);

			if(i != 4-1)
				HAL_UART_Transmit(&huart2, (uint8_t*)",", 1, HAL_MAX_DELAY);
		}

		// ----- Separator -----
		HAL_UART_Transmit(&huart2, (uint8_t*)"|", 1, HAL_MAX_DELAY);

		// ----- Fault -----
		uint8_to_hex2(temp, slaveFaultS[board_num-1]);
		for(int j=0;j<2;j++)
			HAL_UART_Transmit(&huart2, (uint8_t*)&temp[j], 1, HAL_MAX_DELAY);

		// ----- End of Frame -----
		HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
	}
}

void Send_GUI_Reading (void)
{

	for(uint8_t i=0;i<TOTALBOARDS;i++)
	{
		UART_Send_Board_Frame_CharByChar(i);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}
//	    const char *msg1 = "S1|1E78,1E79,0000,0000,0000,0000,1E78,1E79,0000,0000,0000,0000,1E78,1E79,0000,0000|0FFF,0000,0FFF,0555|54\n";
//		const char *msg2 = "S2|1E78,1E79,0000,0000,0000,0000,1E78,1E79,0000,0000,0000,0000,1E78,1E79,0000,0000|0FFF,0000,0FFF,0555|54\n";
//		const char *msg3 = "S3|1E78,1E79,0000,0000,0000,0000,1E78,1E79,0000,0000,0000,0000,1E78,1E79,0000,0000|0FFF,0000,0FFF,0555|54\n";
