#include "BMS_Config.h"

extern CAN_HandleTypeDef hcan1;

//Sets up can Filter, notification, and starts CAN
void BMS_Can_Init(void)
{
       CAN_FilterTypeDef canFilterConfig;
       uint32_t filter_id   = (CAN_CH_TO_BMS << 3) | (1 << 2);  // Extended ID + IDE bit
       	uint32_t filter_mask = (0x1FFFFFFF << 3) | (1 << 2);  // Match all 29 ID bits + IDE

       canFilterConfig.FilterBank = 0;
       canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
       canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
       canFilterConfig.FilterIdHigh = (filter_id >> 16) & 0xFFFF;
       canFilterConfig.FilterIdLow  = filter_id & 0xFFFF;
       //Accept All IDs
       canFilterConfig.FilterMaskIdHigh = (filter_mask >> 16) & 0xFFFF;
       canFilterConfig.FilterMaskIdLow  = filter_mask & 0xFFFF;

       canFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
       canFilterConfig.FilterActivation = ENABLE;
       canFilterConfig.SlaveStartFilterBank = 14;

       if(HAL_CAN_ConfigFilter(&hcan1, &canFilterConfig) != HAL_OK)
       {
               Error_Handler();
       }

       HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

       //Page no #96
       if (HAL_CAN_Start(&hcan1) != HAL_OK)
       {
               Error_Handler();
       }


}

uint8_t slaveCellCount[SLAVEBOARDS] = {16,16 };
void initSlaveCellCount(void) {
    for (uint8_t s = 0; s < SLAVEBOARDS; s++) {
        if (s % 2 == 0) {
            slaveCellCount[s] = 16;  // even index = 14 cells
        } else {
            slaveCellCount[s] = 16;  // odd index  = 13 cells
        }
    }
}
