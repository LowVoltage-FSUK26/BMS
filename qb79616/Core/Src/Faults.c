/*
 * Faults.c
 *
 *  Created on: Mar 15, 2026
 *      Author: AbdUllah
 */


#include "stm32f1xx_hal.h"
#include "bq79616.h"
#include "bq79600.h"
#include "BMS_Config.h"

#include "event_groups.h"

EventGroupHandle_t faultEventGroup;

#define FAULT_EVENT_BIT   (1 << 0)

void Bridge_FaultInit(void)
{
	uint8_t bridge_faultMask = 0x00;   // Mask FTONE + HB + FCOMM

	writeReg(0, Bridge_FAULT_MSK, bridge_faultMask, 1, FRMWRT_SGL_W);

	uint8_t bridge_rst = 0xFF;   // Reset all fault groups
	writeReg(0, Bridge_FAULT_RST, bridge_rst, 1, FRMWRT_SGL_W);


}

uint8_t bridge_faultSummary = 0;

uint8_t bridge_faultPWR   = 0;
uint8_t bridge_faultSYS   = 0;
uint8_t bridge_faultREG   = 0;
uint8_t bridge_faultCOMM1 = 0;
uint8_t bridge_faultCOMM2 = 0;

void Bridge_CheckFaults(void)
{
	bridge_faultSummary = 0;

	bridge_faultPWR   = 0;
	bridge_faultSYS   = 0;
	bridge_faultREG   = 0;
	bridge_faultCOMM1 = 0;
	bridge_faultCOMM2 = 0;

	/* Read Level 1 Fault Summary */
	readReg(0, Bridge_FAULT_SUMMARY,
			&bridge_faultSummary, 1, 100, FRMWRT_SGL_R);

	if(bridge_faultSummary == 0)
	{
		// No bridge faults detected
		return;
	}

	/**************** POWER FAULTS ****************/
	if(bridge_faultSummary & 0x01)
	{



		if(bridge_faultPWR & (1 << 1))   // CVDD_OV
		{
			// Handle CVDD overvoltage
		}

		if(bridge_faultPWR & (1 << 2))   // DVDD_OV
		{
			// Handle DVDD overvoltage
		}
	}

	/**************** SYSTEM FAULTS ****************/
	if(bridge_faultSummary & 0x02)
	{
		readReg(0, Bridge_FAULT_SYS,
				&bridge_faultSYS, 1, 100, FRMWRT_SGL_R);

		if(bridge_faultSYS & (1 << 4))   // DRST
		{
			// Digital reset occurred
		}

		if(bridge_faultSYS & (1 << 2))   // SHUTDOWN_REC
		{
			// Shutdown recovery detected
		}

		if(bridge_faultSYS & (1 << 3))   // CTL
		{
			// Long communication timeout
		}

		if(bridge_faultSYS & (1 << 5))   // CTS
		{
			// Short communication timeout
		}
	}

	/**************** REGISTER FAULTS ****************/
	if(bridge_faultSummary & 0x04)
	{
		readReg(0, Bridge_FAULT_REG,
				&bridge_faultREG, 1, 100, FRMWRT_SGL_R);

		if(bridge_faultREG & (1 << 0))   // FACT_CRC
		{
			// Factory CRC error
		}

		if(bridge_faultREG & (1 << 1))   // FACTLDERR
		{
			// Factory load error
		}

		if(bridge_faultREG & (1 << 2))   // CONF_MON_ERR
		{
			// Config bit flip detected
		}
	}

	/**************** COMMUNICATION FAULTS ****************/
	if(bridge_faultSummary & 0x08)
	{
		readReg(0, Bridge_FAULT_COMM1,
				&bridge_faultCOMM1, 1, 100, FRMWRT_SGL_R);

		readReg(0, Bridge_FAULT_COMM2,
				&bridge_faultCOMM2, 1, 100, FRMWRT_SGL_R);

		/* COMM1 Faults */

		if(bridge_faultCOMM1 & (1 << 6))   // FCOMM_DET
		{
			// Stack device fault detected
		}

		if(bridge_faultCOMM1 & (1 << 5))   // FTONE_DET
		{
			// Fault tone detected
		}

		if(bridge_faultCOMM1 & (1 << 4))   // HB_FAIL
		{
			// Heartbeat missing
		}

		if(bridge_faultCOMM1 & (1 << 3))   // HB_FAST
		{
			// Heartbeat too fast
		}

		if(bridge_faultCOMM1 & (1 << 2))   // UART_FRAME
		{
			// UART frame error
		}

		/* COMM2 Faults */

		if(bridge_faultCOMM2 & (1 << 5))   // SPI_FRAME
		{
			// SPI frame fault
		}

		if(bridge_faultCOMM2 & (1 << 4))   // SPI_PHY
		{
			// SPI physical fault
		}

		if(bridge_faultCOMM2 & (1 << 3))   // COML_FRAME
		{
			// Daisy chain byte fault
		}

		if(bridge_faultCOMM2 & (1 << 2))   // COML_PHY
		{
			// Daisy chain bit fault
		}

		if(bridge_faultCOMM2 & (1 << 1))   // COMH_FRAME
		{
			// High side byte fault
		}

		if(bridge_faultCOMM2 & (1 << 0))   // COMH_PHY
		{
			// High side bit fault
		}
	}

	/**************** RESET SELECTED FAULTS ****************/

	uint8_t bridge_resetCmd = 0;

	/* Reset COMM faults */
	bridge_resetCmd |= (1 << 6);   // RST_UART_SPI
	bridge_resetCmd |= (1 << 4);   // RST_FTONE_DET
	bridge_resetCmd |= (1 << 3);   // RST_FCOMM_DET

	/* Reset SYS faults */
	bridge_resetCmd |= (1 << 1);   // RST_SYS

	/* Reset PWR faults */
	bridge_resetCmd |= (1 << 0);   // RST_PWR

	writeReg(0, Bridge_FAULT_RST,
			0xFF, 1, FRMWRT_SGL_W);
}




void Slave_FaultInit(uint8_t slaveID)
{
	uint8_t rst1 = 0xFF;
	uint8_t rst2 = 0x7F;

	uint8_t msk1 = 0x87;
	uint8_t msk2 = 0xff;
	/* Clear faults */
	writeReg(slaveID, FAULT_RST1, rst1, 1, FRMWRT_SGL_W);
	writeReg(slaveID, FAULT_RST2, rst2, 1, FRMWRT_SGL_W);

	/* Configure masks */
	writeReg(slaveID, BQ79616_FAULT_MSK1, msk1, 1, FRMWRT_SGL_W);
	writeReg(slaveID, BQ79616_FAULT_MSK2, msk2, 1, FRMWRT_SGL_W);


}

void Stack_FaultInit(void)
{
	for(uint8_t id = 1; id <= SLAVEBOARDS; id++)
	{
		Slave_FaultInit(id);
	}
}




/* FAULT SUMMARY BITS */

#define SLAVE_FLT_PROT        (1 << 7)
#define SLAVE_FLT_COMP_ADC    (1 << 6)
#define SLAVE_FLT_OTP         (1 << 5)
#define SLAVE_FLT_COMM        (1 << 4)
#define SLAVE_FLT_OTUT        (1 << 3)
#define SLAVE_FLT_OVUV        (1 << 2)
#define SLAVE_FLT_SYS         (1 << 1)
#define SLAVE_FLT_PWR         (1 << 0)

uint8_t slaveFaultS[SLAVEBOARDS];

void Slave_CheckFaultSummary(uint8_t slaveID)
{
	uint8_t slaveFaultSummary = 0;

	/* Read FAULT SUMMARY */
	readReg(slaveID,
			FAULT_SUMMARY,
			&slaveFaultSummary,
			1,
			100,
			FRMWRT_SGL_R);

	slaveFaultS[slaveID-1]=slaveFaultSummary ;
	/* No Fault */
	if(slaveFaultSummary == 0)
	{
		return;
	}

	/*************** POWER ***************/
	if(slaveFaultSummary & SLAVE_FLT_PWR)
	{
		// Read FAULT_PWR1/2/3 later
	}

	/*************** SYSTEM ***************/
	if(slaveFaultSummary & SLAVE_FLT_SYS)
	{
		// Read FAULT_SYS
	}

	/*************** OV / UV ***************/
	if(slaveFaultSummary & SLAVE_FLT_OVUV)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		vTaskDelay(5000);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
		vTaskDelay(5000);

		// Read FAULT_OV1/2 + FAULT_UV1/2
	}

	/*************** OT / UT ***************/
	if(slaveFaultSummary & SLAVE_FLT_OTUT)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		vTaskDelay(5000);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		vTaskDelay(5000);
		// Read FAULT_OT1/2 + FAULT_UT1/2
	}

	/*************** COMM ***************/
	if(slaveFaultSummary & SLAVE_FLT_COMM)
	{
		// Read FAULT_COMM1/2/3
	}

	/*************** OTP ***************/
	if(slaveFaultSummary & SLAVE_FLT_OTP)
	{
		// Read FAULT_OTP
	}

	/*************** ADC COMP ***************/
	if(slaveFaultSummary & SLAVE_FLT_COMP_ADC)
	{
		// Read FAULT_COMP_* registers
	}

	/*************** PROTECTION ***************/
	if(slaveFaultSummary & SLAVE_FLT_PROT)
	{
		// Read FAULT_PROT1/2
	}

	writeReg(slaveID, FAULT_RST1, 0xff, 1, FRMWRT_SGL_W);
	writeReg(slaveID, FAULT_RST2, 0xff, 1, FRMWRT_SGL_W);
}
void Stack_CheckFaultSummary(void)
{
	for(uint8_t id = 1; id <= SLAVEBOARDS; id++)
	{
		Slave_CheckFaultSummary(id);
	}
}





//nfault interrupt handling
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN)
{
	if(GPIO_PIN== GPIO_PIN_8)
	{

		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		xEventGroupSetBitsFromISR(
				faultEventGroup,
				FAULT_EVENT_BIT,
				&xHigherPriorityTaskWoken
		);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	}
}

//while(1)
//		{
//			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
//					vTaskDelay(1000);
//					HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
//					vTaskDelay(1000);
//
//		}
