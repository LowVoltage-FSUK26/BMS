/*
 * BMS_Config.c
 *
 *  Created on: Feb 22, 2026
 *      Author: Mina Fathy
 */

#include "BMS_Config.h"


extern CAN_HandleTypeDef hcan;

//Sets up can Filter, notification, and starts CAN
void BMS_Can_Init(void)
{
	CAN_FilterTypeDef canFilterConfig;
	canFilterConfig.FilterBank = 0;
	canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	canFilterConfig.FilterIdHigh = 0x0000;
	canFilterConfig.FilterIdLow = 0x0000;
	//Accept All IDs
	canFilterConfig.FilterMaskIdHigh = 0x0000;
	canFilterConfig.FilterMaskIdLow = 0x0000;
	canFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	canFilterConfig.FilterActivation = ENABLE;
	canFilterConfig.SlaveStartFilterBank = 14;

	if(HAL_CAN_ConfigFilter(&hcan, &canFilterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

	//Page no #96
	if (HAL_CAN_Start(&hcan) != HAL_OK)
	{
		Error_Handler();
	}
}

