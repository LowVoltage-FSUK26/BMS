 /* stm32_bq79616.c
 *
 * This code is adapted from a TI sample for the BQ79616 battery monitoring IC.
 * It has been modified to use the STM32 HAL library (for the Blue Pill board)
 * and uses UART communication via huart1.
 *
 * Make sure that:
 *   - huart1 is properly defined and initialized elsewhere in your project.
 *   - The GPIO pin used for TX (here assumed PA9) is correctly defined.
 * 
 * Note: this version uses blocking UART recieving, 
 *      it doesn't work with multiple boards,
 *      for 1 board: 5us delay is needed between sending command and recieving the response
 *
 * Some functions are not completely implemented and still need to be modified and tested
 * so refer to bq79616 for exact configurations details.
 * 
 * Created By: Aya Yasser
 */

#include "stm32f1xx_hal.h"    
#include <string.h>
#include "BMS_Config.h"
#include "event_groups.h"


// External UART handle (assumed to be defined and initialized in main.c)
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2; //huart2 is used only for debugging
extern UBaseType_t uxHighWaterMark;
extern uint8_t init_done;

// Global variables (adjust sizes as needed)
uint8_t response_frame2[(MAXBYTES+6)*TOTALBOARDS];
unsigned char autoaddr_response_frame[(1+6)*TOTALBOARDS]; //response frame for auto-addressing sequence

uint8_t fullBuffer[(MAXBYTES+6)*TOTALBOARDS];
uint8_t fault_frame[39*TOTALBOARDS];
int currentBoard = 0;
int currentCell = 0;
uint8_t bReturn = 0;
int bRes = 0;
int count = 10000;
uint8_t bBuf[8];
uint8_t pFrame[64];
uint16_t wCRC = 0;
uint16_t wCRC16 = 0;
int crc_i = 0;

uint8_t int_ack=0;

/* 
 * A microsecond delay function.
 *
 * Note: HAL_Delay() provides millisecond delays. For sub-millisecond delays,
 * you can implement delay_us() using a hardware timer or the DWT cycle counter.
 * The following is a simple example using the DWT (make sure to enable it at startup).
 */
// Helper: Delay in microseconds.
uint32_t gSysTickLoad = 0;
uint32_t gTicksPerMicrosecondFloor = 0;
uint32_t gTicksPerMicrosecondMod1000 = 0;

void DELAY_init(void)
{
	//== Prep for DELAY_microseconds() ==

	//SysTick->LOAD gets set to the number of ticks in a millisecond, in HAL_SYSTICK_Config(), which
	//is called from HAL_Init(). So we don't need to determine or assume clock frequency, and can
	//instead just work backwards from there.
	gSysTickLoad                = SysTick->LOAD;
	gTicksPerMicrosecondFloor   = (gSysTickLoad+1) / 1000;
	//This will be zero unless the clock is not an even number of MHz.
	gTicksPerMicrosecondMod1000 = (gSysTickLoad+1) % 1000;
}
void inline __attribute__((always_inline)) DELAY_microseconds(uint16_t us)
												{
	uint32_t startTick = SysTick->VAL; //get start tick as soon as possible

	//The asserts and division that follow dictate the minimum possible delay (smallest
	//supported "us" parameter). They provide sanity checks and accuracy as a trade-off.
	//assert(gSysTickLoad == SysTick->LOAD); //make sure DELAY_init got called and nothing has changed since
	//assert(us < 1000); //otherwise, infinite loop!

	uint32_t delayTicks = gTicksPerMicrosecondFloor * us;
	delayTicks += (500 + gTicksPerMicrosecondMod1000 * us) / 1000; //Probably a nop

	while(1)
	{
		uint32_t currentTicks = SysTick->VAL;
		//Handle SysTick->VAL hitting zero and resetting back to SysTick->LOAD.
		uint32_t elapsedTicks = currentTicks < startTick ? startTick - currentTicks :
				gSysTickLoad + startTick - currentTicks;
		if(elapsedTicks >= delayTicks)
			break;
	}
												}

/* 
 * The helping functions.
 *
 * For each, we first deinitialize the UART (thus releasing the TX pin),
 * reconfigure the TX pin (here PA9) as a GPIO output and drive it low for
 * the specified period. Then we reinitialize the UART so that normal
 * communication can resume.
 *
 * Adjust pin names/ports as needed.
 */
void Wake79616(void)
{
	HAL_UART_DeInit(&huart1);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();   // Make sure the clock is enabled for GPIOA

	// Configure PA9 (UART TX) as a push-pull output.
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

	//HAL_Delay(1000);

	// Drive TX low
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	// Delay 2.5 ms (using HAL_Delay which works in ms)

	HAL_Delay(2);

	// Reinitialise UART (this call should reconfigure PA9 to its alternate function)
	HAL_UART_Init(&huart1);
}

void Wake79600(void)
{
	uint8_t received_data = 0;
	HAL_UART_DeInit(&huart1);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();   // Make sure the clock is enabled for GPIOA

	// Configure PA9 (UART TX) as a push-pull output.
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);


	// Drive TX low
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	HAL_Delay(2); // WAKE ping = 2.5ms to 3ms

	// Reinitialize UART (this call should reconfigure PA9 to its alternate function)
	HAL_UART_Init(&huart1);

	//Needed Delay for UART of bridge to work
	HAL_Delay(3);
}

void Wake79600_RTOS(void)
{
	uint8_t received_data = 0;
	HAL_UART_DeInit(&huart1);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();   // Make sure the clock is enabled for GPIOA

	// Configure PA9 (UART TX) as a push-pull output.
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);


	// Drive TX low
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	vTaskDelay(2); // WAKE ping = 2.5ms to 3ms

	// Reinitialize UART (this call should reconfigure PA9 to its alternate function)
	HAL_UART_Init(&huart1);

	//Needed Delay for UART of bridge to work
	vTaskDelay(3);
}

void SD79616(void)
{
	HAL_UART_DeInit(&huart1);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	// Delay 9 ms
	HAL_Delay(9);

	HAL_UART_Init(&huart1);
}

void StA79616(void)
{
	HAL_UART_DeInit(&huart1);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	// Delay 250 microseconds (using delay_us)
	DELAY_microseconds(250);
	HAL_UART_Init(&huart1);
}

void HWRST79616(void)
{
	HAL_UART_DeInit(&huart1);

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	// Delay 36 ms
	HAL_Delay(36);

	HAL_UART_Init(&huart1);
}

// auto addressing sequence for daisy chain **NO BRIDGE**
void AutoAddress(void)
{
	//DUMMY WRITE TO SNCHRONIZE ALL DAISY CHAIN DEVICES DLL (IF A DEVICE RESET OCCURED PRIOR TO THIS)
	writeReg(0, OTP_ECC_DATAIN1, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN2, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN3, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN4, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN5, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN6, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN7, 0X00, 1, FRMWRT_ALL_W);
	writeReg(0, OTP_ECC_DATAIN8, 0X00, 1, FRMWRT_ALL_W);

	//writeReg(0, OTP_ECC_DATAIN9, 0X00, 1, FRMWRT_ALL_W);
	//writeReg(0, OTP_ECC_TEST, 0X00, 1, FRMWRT_ALL_W);

	//ENABLE AUTO ADDRESSING MODE
	writeReg(0, BQ79616_CONTROL1, 0X01, 1, FRMWRT_ALL_W);

	//SET ADDRESSES FOR EVERY BOARD
	for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
	{
		writeReg(0, BQ79616_DIR0_ADDR, currentBoard, 1, FRMWRT_ALL_W);
	}

	writeReg(0, BQ79616_COMM_CTRL, 0x02, 1, FRMWRT_ALL_W); //set everything as a stack device first

	if(TOTALBOARDS==1) //if there's only 1 board, it's the base AND top of stack, so change it to those
	{
		writeReg(0, BQ79616_COMM_CTRL, 0x01, 1, FRMWRT_SGL_W);
	}
	else //otherwise set the base and top of stack individually
	{
		writeReg(0, BQ79616_COMM_CTRL, 0x00, 1, FRMWRT_SGL_W);
		writeReg(TOTALBOARDS-1, BQ79616_COMM_CTRL, 0x03, 1, FRMWRT_SGL_W);
	}

	//SYNCRHONIZE THE DLL WITH A THROW-AWAY READ
	readReg(0, OTP_ECC_DATAIN1, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN2, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN3, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN4, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN5, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN6, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN7, response_frame2, 1, 0, FRMWRT_ALL_R);
	readReg(0, OTP_ECC_DATAIN8, response_frame2, 1, 0, FRMWRT_ALL_R);


	//OPTIONAL: read back all device addresses
	for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
	{
		readReg(currentBoard, BQ79616_DIR0_ADDR, response_frame2, 1, 0, FRMWRT_SGL_R);

	}

	//RESET ANY COMM FAULT CONDITIONS FROM STARTUP
	writeReg(0, FAULT_RST2, 0x03, 1, FRMWRT_ALL_W);

	return;
}

// auto addressing sequence for daisy chain **WITH BRIDGE**
void Bridge_AutoAddress(void)
{
	//DUMMY WRITE TO SNCHRONIZE ALL DAISY CHAIN DEVICES DLL (IF A DEVICE RESET OCCURED PRIOR TO THIS)
	writeReg(0, OTP_ECC_DATAIN1, 0X00, 1, FRMWRT_STK_W);	//Modification: changed FRMWRT_ALL_W to FRMWRT_STK_W
	writeReg(0, OTP_ECC_DATAIN2, 0X00, 1, FRMWRT_STK_W);
	writeReg(0, OTP_ECC_DATAIN3, 0X00, 1, FRMWRT_STK_W);
	writeReg(0, OTP_ECC_DATAIN4, 0X00, 1, FRMWRT_STK_W);
	writeReg(0, OTP_ECC_DATAIN5, 0X00, 1, FRMWRT_STK_W);
	writeReg(0, OTP_ECC_DATAIN6, 0X00, 1, FRMWRT_STK_W);
	writeReg(0, OTP_ECC_DATAIN7, 0X00, 1, FRMWRT_STK_W);
	writeReg(0, OTP_ECC_DATAIN8, 0X00, 1, FRMWRT_STK_W);


	//ENABLE AUTO ADDRESSING MODE
	writeReg(0, BQ79616_CONTROL1, 0X01, 1, FRMWRT_ALL_W);

	//SET ADDRESSES FOR EVERY BOARD
	for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
	{
		writeReg(0, BQ79616_DIR0_ADDR, currentBoard, 1, FRMWRT_ALL_W);
	}

	//BROADCAST WRITE TO SET ALL DEVICES AS STACK DEVICE
	writeReg(0, BQ79616_COMM_CTRL, 0x02, 1, FRMWRT_ALL_W); //Check: Change FRMWRT_ALL_W to FRMWRT_STK_W

	//SET THE HIGHEST DEVICE IN THE STACK AS BOTH STACK AND TOP OF STACK
	writeReg(TOTALBOARDS-1, BQ79616_COMM_CTRL, 0x03, 1, FRMWRT_SGL_W);


	//OPTIONAL: read back all device addresses on the Stack
	//Future Modification: Change to stack read
	for(currentBoard=1; currentBoard<TOTALBOARDS; currentBoard++)
	{
		readReg(currentBoard, BQ79616_DIR0_ADDR, response_frame2, 1, 1000, FRMWRT_SGL_R);

	}
	//OPTIONAL: read register address 0x2001 and verify that the value is 0x14
	readReg(0, 0x2001, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);

	//SYNCRHONIZE THE DLL WITH A THROW-AWAY READ
	readReg(1, OTP_ECC_DATAIN1, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(2, OTP_ECC_DATAIN2, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(1, OTP_ECC_DATAIN3, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(2, OTP_ECC_DATAIN4, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(1, OTP_ECC_DATAIN5, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(2, OTP_ECC_DATAIN6, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(1, OTP_ECC_DATAIN7, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);
	readReg(2, OTP_ECC_DATAIN8, autoaddr_response_frame, 1, 0, FRMWRT_SGL_R);

	return;
}


//Auto Addressing sequence for Ring Configuration
void AutoAddress_Ring(void)
{
	AutoAddress();

	//Reverse Communication Direction of Base
	writeReg(0, BQ79616_CONTROL1, (1 << 7), 1, FRMWRT_SGL_W);

	//Revese Communication Direction for the reset of chain
	writeReg(0, BQ79616_CONTROL1, (1 << 7), 1, FRMWRT_ALL_R);

	//SET ADDRESSES FOR EVERY BOARD
	for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
	{
		writeReg(0, BQ79616_DIR1_ADDR, currentBoard, 1, FRMWRT_ALL_W);
	}

	writeReg(0, BQ79616_COMM_CTRL, 0x02, 1, FRMWRT_ALL_W); //set everything as a stack device first

	if(TOTALBOARDS==1) //if there's only 1 board, it's the base AND top of stack, so change it to those
	{
		writeReg(0, BQ79616_COMM_CTRL, 0x01, 1, FRMWRT_SGL_W);
	}
	else //otherwise set the base and top of stack individually
	{
		writeReg(0, BQ79616_COMM_CTRL, 0x00, 1, FRMWRT_SGL_W);
		writeReg(TOTALBOARDS-1, BQ79616_COMM_CTRL, 0x03, 1, FRMWRT_SGL_W);
	}

	//return to original Communication Direction of Base
	writeReg(0, BQ79616_CONTROL1, (0 << 7), 1, FRMWRT_SGL_W);

	//return to original Communication Direction for the reset of chain
	writeReg(0, BQ79616_CONTROL1, (0 << 7), 1, FRMWRT_ALL_R);

	//SYNCRHONIZE THE DLL WITH A THROW-AWAY READ
	//    readReg(0, OTP_ECC_DATAIN1, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN2, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN3, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN4, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN5, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN6, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN7, response_frame2, 1, 0, FRMWRT_ALL_R);
	//    readReg(0, OTP_ECC_DATAIN8, response_frame2, 1, 0, FRMWRT_ALL_R);
	//
	//     //OPTIONAL: read back all device addresses
	//    for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
	//    {
	//        readReg(currentBoard, BQ79616_DIR0_ADDR, response_frame2, 1, 0, FRMWRT_SGL_R);
	//
	//    }

	//RESET ANY COMM FAULT CONDITIONS FROM STARTUP
	writeReg(0, FAULT_RST2, 0x03, 1, FRMWRT_ALL_W);


}

/*
 * WriteReg: Format the write data and send it out.
 * (This function remains largely unchanged, except that it now calls WriteFrame()
 * which uses HAL_UART_Transmit.)
 */
int writeReg(uint8_t bID, uint16_t wAddr, uint64_t dwData, uint8_t bLen, uint8_t bWriteType)
{
	bRes = 0;
	memset(bBuf, 0, sizeof(bBuf));

	switch (bLen) {
	case 1:
		bBuf[0] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 1, bWriteType);
		break;
	case 2:
		bBuf[0] = (dwData >> 8) & 0xFF;
		bBuf[1] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 2, bWriteType);
		break;
	case 3:
		bBuf[0] = (dwData >> 16) & 0xFF;
		bBuf[1] = (dwData >> 8) & 0xFF;
		bBuf[2] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 3, bWriteType);
		break;
	case 4:
		bBuf[0] = (dwData >> 24) & 0xFF;
		bBuf[1] = (dwData >> 16) & 0xFF;
		bBuf[2] = (dwData >> 8) & 0xFF;
		bBuf[3] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 4, bWriteType);
		break;
	case 5:
		bBuf[0] = (dwData >> 32) & 0xFF;
		bBuf[1] = (dwData >> 24) & 0xFF;
		bBuf[2] = (dwData >> 16) & 0xFF;
		bBuf[3] = (dwData >> 8) & 0xFF;
		bBuf[4] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 5, bWriteType);
		break;
	case 6:
		bBuf[0] = (dwData >> 40) & 0xFF;
		bBuf[1] = (dwData >> 32) & 0xFF;
		bBuf[2] = (dwData >> 24) & 0xFF;
		bBuf[3] = (dwData >> 16) & 0xFF;
		bBuf[4] = (dwData >> 8) & 0xFF;
		bBuf[5] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 6, bWriteType);
		break;
	case 7:
		bBuf[0] = (dwData >> 48) & 0xFF;
		bBuf[1] = (dwData >> 40) & 0xFF;
		bBuf[2] = (dwData >> 32) & 0xFF;
		bBuf[3] = (dwData >> 24) & 0xFF;
		bBuf[4] = (dwData >> 16) & 0xFF;
		bBuf[5] = (dwData >> 8) & 0xFF;
		bBuf[6] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 7, bWriteType);
		break;
	case 8:
		bBuf[0] = (dwData >> 56) & 0xFF;
		bBuf[1] = (dwData >> 48) & 0xFF;
		bBuf[2] = (dwData >> 40) & 0xFF;
		bBuf[3] = (dwData >> 32) & 0xFF;
		bBuf[4] = (dwData >> 24) & 0xFF;
		bBuf[5] = (dwData >> 16) & 0xFF;
		bBuf[6] = (dwData >> 8) & 0xFF;
		bBuf[7] = dwData & 0xFF;
		bRes = writeFrame(bID, wAddr, bBuf, 8, bWriteType);
		break;
	default:
		break;
	}
	return bRes;
}

/*
 * WriteFrame: Generate the command frame and send it via UART.
 * The TI codes sciSend is replaced by HAL_UART_Transmit.
 */
int writeFrame(uint8_t bID, uint16_t wAddr, uint8_t * pData, uint8_t bLen, uint8_t bWriteType)
{
	int bPktLen = 0;
	uint8_t * pBuf = pFrame;

	// Fill pFrame with 0x7F (as in the original)
	memset(pFrame, 0x7F, sizeof(pFrame));

	// First byte: combine header bits
	*pBuf++ = 0x80 | (bWriteType) | ((bWriteType & 0x10) ? (bLen - 1) : 0);

	// For single read/write, include device ID
	if (bWriteType == FRMWRT_SGL_R || bWriteType == FRMWRT_SGL_W)
	{
		*pBuf++ = (bID & 0xFF);
	}

	// Add the register address (two bytes)
	*pBuf++ = (wAddr >> 8) & 0xFF;
	*pBuf++ = wAddr & 0xFF;

	// Append data bytes
	for (int i = 0; i < bLen; i++)
	{
		*pBuf++ = pData[i];
	}

	bPktLen = pBuf - pFrame;

	// Compute the CRC over the frame so far.
	wCRC = CRC16(pFrame, bPktLen);
	*pBuf++ = wCRC & 0xFF;
	*pBuf++ = (wCRC >> 8) & 0xFF;
	bPktLen += 2;

	// Transmit the frame using HAL_UART_Transmit (with a timeout of 100 ms)
	HAL_UART_Transmit(&huart1, pFrame, bPktLen, 100);


	return bPktLen;
}

/* Event Group */
EventGroupHandle_t uartEventGroup;
#define UART1_RX_DONE_BIT   (1 << 0)

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;

		xEventGroupSetBitsFromISR(uartEventGroup, UART1_RX_DONE_BIT, &xHigherPriorityTaskWoken);

		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

volatile uint32_t ms_counter = 0;



/*
 * ReadReg: Generate a read command frame, send it, then wait for a response.
 * This version uses blocking HAL_UART_Receive.
 * note: this function only works for single read (FRMWRT_SGL_R)
 *      other types of read still need to be modified and tested
 */

int readReg(uint8_t bID, uint16_t wAddr, uint8_t* pData, uint8_t bLen, uint32_t dwTimeOut, uint8_t bWriteType) {
	int bRes = 0;
	memset(fullBuffer, 0, sizeof(fullBuffer));

	// Generate Read Frame Request
	if (bWriteType == FRMWRT_SGL_R) {
		readFrameReq(bID, wAddr, bLen, bWriteType);

		HAL_UART_Receive_IT(&huart1, fullBuffer, bLen + 6);
		bRes = bLen + 6;
	} else if (bWriteType == FRMWRT_STK_R) {
		readFrameReq(bID, wAddr, bLen, bWriteType);

		HAL_UART_Receive_IT(&huart1, fullBuffer, (bLen + 6) * (TOTALBOARDS - 1));
		bRes = (bLen + 6) * (TOTALBOARDS - 1);
	} else if (bWriteType == FRMWRT_ALL_R) {
		readFrameReq(bID, wAddr, bLen, bWriteType);

		HAL_UART_Receive_IT(&huart1, fullBuffer, (bLen + 6) * TOTALBOARDS);
		bRes = (bLen +6) * TOTALBOARDS;
	} else {
		return 0; // Invalid read type
	}

	EventBits_t bits = xEventGroupWaitBits(
			uartEventGroup,
			UART1_RX_DONE_BIT,
			pdTRUE,       // Clear bit on exit
			pdTRUE,       // Wait for all bits
			pdMS_TO_TICKS(1000)
	);

	if(bits & UART1_RX_DONE_BIT)
	{
		// **Check CRC for data integrity**
		if (CRC16(fullBuffer, bLen + 6) != 0) {
			return 0;
		}
		// **Extract actual register data from the received buffer**
		memcpy(pData, &fullBuffer[4], bLen);
	}
	else
	{
		HAL_NVIC_SystemReset();
	}

	return bRes;  // Return number of valid data bytes extracted
}

/*
 * ReadFrameReq: Generate a read command frame.
 */
int readFrameReq(uint8_t bID, uint16_t wAddr, uint8_t bByteToReturn, uint8_t bWriteType)
{
	bReturn = bByteToReturn - 1;
	if (bReturn > 127)
		return 0;

	return writeFrame(bID, wAddr, &bReturn, 1, bWriteType);
}

/*
 * CRC16: Calculate the CRC using the provided table.
 * (The CRC table is the same as in the TI code.)
 */
// CRC16 TABLE
// ITU_T polynomial: x^16 + x^15 + x^2 + 1
const uint16_t crc16_table[256] = { 0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301,
		0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1,
		0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81,
		0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00,
		0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1,
		0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380,
		0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141,
		0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501,
		0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0,
		0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881,
		0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401,
		0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1,
		0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0, 0x6180,
		0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740,
		0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01,
		0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1,
		0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80,
		0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200,
		0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1,
		0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780,
		0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41,
		0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901,
		0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1,
		0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80,
		0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 };

uint16_t CRC16(uint8_t *pBuf, int nLen)
{
	uint16_t wCRC = 0xFFFF;
	for (int i = 0; i < nLen; i++)
	{
		wCRC ^= pBuf[i];
		wCRC = crc16_table[wCRC & 0xFF] ^ (wCRC >> 8);
	}
	return wCRC;
}

//RUN BASIC CELL BALANCING FOR ALL DEVICES
void RunCB()
{
	//SET BALANCING TIMERS TO 30 s
	writeReg(0, BQ79616_CB_CELL_CTRL_16, 0x0202020202020202, 8, FRMWRT_ALL_W);   //cell 16-9 (8 byte max write)
	writeReg(0, BQ79616_CB_CELL_CTRL_08, 0x0202020202020202, 8, FRMWRT_ALL_W);    //cell 8-1

	//SET DUTY CYCLE TO 10 s (default)
	writeReg(0, BQ79616_BAL_CTRL1, 0x01, 1, FRMWRT_ALL_W);   //10s duty cycle

	//START BALANCING
	writeReg(0, BQ79616_BAL_CTRL2, 0x03, 1, FRMWRT_ALL_W); //auto balance and BAL_GO
}

void update_balancing_threshold(float min_voltage) {
	uint8_t threshold_value = (uint8_t)((min_voltage - 2.8) / 0.025);
	writeReg(0, BQ79616_VCB_DONE_THRESH, threshold_value, 1, FRMWRT_ALL_W);
	writeReg(0, BQ79616_OVUV_CTRL, 0x05, 1, FRMWRT_ALL_W);          //round-robin and OVUV_GO
}

void check_cell_balancing(uint8_t boardNum, uint8_t cellsNum){
	float voltages[16];
	float max_voltage = 0, min_voltage = 100;
	uint8_t balance_mask = 0x00;  // Bitmask for which cells to balance

	// **1. Read all cell voltages**
	// **2. Check if balancing should start**
	// check (max_voltage - min_voltage) > BALANCING_THRESHOLD
	// check SoC > SOC_BALANCING_START
	// check temp < TEMP_LIMIT
	// enable balancing for cells above the threshold only:
	/*
			for (int i = 0; i < 16; i++) {
					if (voltages[i] > (min_voltage + BALANCING_THRESHOLD)) {
					balance_mask |= (1 << i);  // Set bit for this cell
				}
			}
			//Write balancing control mask to BQ79616**
	 */


}

//fault related functions
void ResetAllFaults(uint8_t bID, uint8_t bWriteType)
{
	//BROADCAST INCLUDES EXTRA FUNCTIONALITY TO OVERWRITE THE CUST_CRC WITH THE CURRENT SETTINGS
	if(bWriteType==FRMWRT_ALL_W)
	{
		//READ THE CALCULATED CUSTOMER CRC VALUES
		readReg(0, CUST_CRC_RSLT_HI, fault_frame, 2, 0, FRMWRT_ALL_R);
		//OVERWRITE THE CRC OF EVERY BOARD IN THE STACK WITH THE CORRECT CRC
		for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
		{
			//THE RETURN FRAME STARTS WITH THE HIGHEST BOARD FIRST, SO THIS WILL WRITE THE HIGHEST BOARD FIRST
			writeReg(TOTALBOARDS-currentBoard-1, BQ79616_CUST_CRC_HI, fault_frame[currentBoard*8+4] << 8 | fault_frame[currentBoard*8+5], 2, FRMWRT_SGL_W);
		}
		//NOW CLEAR EVERY FAULT
		writeReg(0, FAULT_RST1, 0xFFFF, 2, FRMWRT_ALL_W);
	}
	else if(bWriteType==FRMWRT_SGL_W)
	{
		writeReg(bID, FAULT_RST1, 0xFFFF, 2, FRMWRT_SGL_W);
	}
	else if(bWriteType==FRMWRT_STK_W)
	{
		writeReg(0, FAULT_RST1, 0xFFFF, 2, FRMWRT_STK_W);
	}
}

void MaskAllFaults(uint8_t bID, uint8_t bWriteType)
{
	if(bWriteType==FRMWRT_ALL_W)
	{
		writeReg(0, BQ79616_FAULT_MSK1, 0xFFFF, 2, FRMWRT_ALL_W);
	}
	else if(bWriteType==FRMWRT_SGL_W)
	{
		writeReg(bID, BQ79616_FAULT_MSK1, 0xFFFF, 2, FRMWRT_SGL_W);
	}
	else if(bWriteType==FRMWRT_STK_W)
	{
		writeReg(0, BQ79616_FAULT_MSK1, 0xFFFF, 2, FRMWRT_STK_W);
	}
}

//enable faults to raise nfault flag
void nfault_enable(){
	//reset conf register to its default value
	uint8_t rstVal = 0x54;
	writeReg(currentBoard, BQ79616_DEV_CONF, 0x54, 1, FRMWRT_SGL_W);
}

void set_VCB_DONE(uint16_t vDone){
	//????????????
}

// Function to configure GPIO1 as specified input
uint8_t configureGPIO(uint8_t GPIO_NUM, BQ79616_GPIO_Config_t GPIO_MODE ,uint8_t BID, uint8_t bWriteType){

	if(BID >= SLAVEBOARDS)
		return 2;		// Invalid Board ID

	if (GPIO_NUM < 1 || GPIO_NUM > 8)
	    return 3;   	// Invalid GPIO

	if (GPIO_MODE > 0x07)
	    return 4;		// Value must fit in 3 bits

	if ((bWriteType != FRMWRT_SGL_W) && (bWriteType != FRMWRT_STK_W) )
	    return 5;		// Invalid bWriteType

	uint8_t reg_addr = BQ79616_GPIO_CONF1 + ((GPIO_NUM - 1)/2);
	uint8_t reg_val = (GPIO_MODE << (!(GPIO_NUM % 2) * 3));
	writeReg((bWriteType == FRMWRT_SGL_W)?BID : 0, reg_addr, reg_val, 1, bWriteType);
	return 1; 			//Correct Config
}

uint8_t configure_OTUT(uint8_t dev_address, uint8_t activeThermistors){

	uint8_t ot_ut = 0xE0;  //reset value UT= 80% and OT= 39%
	uint8_t cb_coolOff= 0x0F; //reset values of OTCB_THR and COOLOFF hysteresis
	uint8_t dev_stat;
	uint8_t goCmd = 0x05;
	uint8_t gpioConf= 0x09; //for simplicity enable all gpio thermistors

	//set UT and OT thresholds
	writeReg(dev_address, BQ79616_OTUT_THRESH, ot_ut ,1 , FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_OTCB_THRESH, cb_coolOff ,1 , FRMWRT_SGL_W);

	//enable TSERF
	writeReg(dev_address, BQ79616_CONTROL2, 0x01, 1, FRMWRT_SGL_W);
	vTaskDelay(2);

	//configure all GPIOs for thermistors
	/*
	writeReg(dev_address, BQ79616_GPIO_CONF1, gpioConf, 1, FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_GPIO_CONF2, gpioConf, 1, FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_GPIO_CONF3, gpioConf, 1, FRMWRT_SGL_W);
	 */

	writeReg(dev_address, BQ79616_GPIO_CONF1, 0x09, 1, FRMWRT_SGL_W);  // Enable GPIO1 for thermistor

	//set OTUT mode
	writeReg(dev_address, BQ79616_OTUT_CTRL, OTUT_MODE,1 , FRMWRT_SGL_W);

	//set Vcb_done to Vuv

	//Start Protection
	writeReg(dev_address, BQ79616_OTUT_CTRL, goCmd ,1 , FRMWRT_SGL_W);

	//read back protection status to ensure its ON
	readReg(dev_address, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
	if((dev_stat& 0x10) == 0){
		return 0;   //error OTUT is not enabled
	}
	return 1;
}


float readGPIOVoltage(uint8_t BID, uint8_t GPIO_NUM, uint16_t* raw_value_ptr) {

	float voltage_uV = 989; //Special value indicating invalid GPIO_NUM
	int16_t raw_value = 0;
	uint16_t buffer[2];
	if((GPIO_NUM >= 1) && (GPIO_NUM <= 8))
	{

		readReg(BID, (GPIO1_HI + 2*(GPIO_NUM - 1)), (uint8_t*)(&buffer[0]), 1, 0, FRMWRT_SGL_R);
		readReg(BID, (GPIO1_LO + 2*(GPIO_NUM - 1)), (uint8_t*)(&buffer[1]), 1, 0, FRMWRT_SGL_R);

		//raw_value = (int16_t)((hi << 8) | lo);
		raw_value =((buffer[1] << 8) | buffer[2]);
		*raw_value_ptr = raw_value;
		voltage_uV = (int16_t)raw_value *VLSB_GPIO/1000000;
	}
	return voltage_uV;
}


