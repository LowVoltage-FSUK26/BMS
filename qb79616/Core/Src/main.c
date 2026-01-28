/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bq79616.h"
#include "bq79600.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

//Define Tasks Handler to hold task ID
TaskHandle_t defaultTaskHandle;

uint8_t received_data1 = 0;
uint8_t received_data2 = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

//Declare Tasks Entery point
void StartDefaultTask(void const * argument);

/* Receive buffer (1 byte) */
//uint8_t rx_byte[5];


/* This callback is called by HAL when a receive complete interrupt occurs */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//  if (huart->Instance == USART1)
//  {
//    /* copy received byte to tx buffer */
//
//
//    /* start transmit interrupt to echo the byte back */
//    if (HAL_UART_Transmit_IT(&huart1, rx_byte, 5) != HAL_OK)
//    {
//      /* Transmission error handling */
//      Error_Handler();
//    }
//
//    /* Re-arm receive interrupt to receive next byte.
//       Important: re-enable after starting TX or you can re-enable in TxCpltCallback.
//    */
//    if (HAL_UART_Receive_IT(&huart1, rx_byte, 5) != HAL_OK)
//    {
//      Error_Handler();
//    }
//  }
//}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void test1(){ //success!!!
	Wake79616();

	HAL_Delay(2);

	AutoAddress();

}
/** Test Case 2: Voltage Reading **/
int totalV = 0;
float final_value;
int cellVoltages_board0[16] = {0};
int cellVoltages_board1[16] = {0};
float test2(){ //success!!!
	int totalV1 = 0;
	int totalV2 = 0;

	uint8_t activeCells = 16;
	// writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_ALL_W);
	readBoardVoltages(1, activeCells, &totalV1, cellVoltages_board0);

	HAL_Delay(100);
	readBoardVoltages(2, activeCells, &totalV2, cellVoltages_board1);
	totalV=totalV1 + totalV2;
	return ((totalV1 + totalV2)*0.00019073);
	/** If this works modify to read more than one board **/
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

int comm_x = 0;
void test4()
{
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

}
uint8_t Buffer[5];


#include <stdint.h>
#include <stdio.h>

// Example register addresses (replace with actual addresses from datasheet)
#define CONTROL2        BQ79616_CONTROL2
#define TSREF_EN_BIT    0x01

#define GPIO_CONF1      BQ79616_GPIO_CONF1
#define GPIO1_ADC_BIT   0x12
#define ADC_CTRL1       BQ79616_ADC_CTRL1
#define MAIN_GO_BIT     0x01

#define GPIO1_HI_REG    GPIO1_HI
#define GPIO1_LO_REG    GPIO1_LO

#define VLSB_GPIO       152.59  // 1 LSB in μV (replace with datasheet value)
#define R1              10000 // Pull-up resistor for thermistor in ohms

#define DIETEMP1_HI_REG DIETEMP1_HI  // High byte register of Die Temperature
#define DIETEMP1_LO_REG DIETEMP1_LO  // Low byte register of Die Temperature
#define VLSB_MAIN_DIETEMP1 0.0078125 // LSB value in °C

int16_t raw_value;
uint8_t hi, lo;

float gpio1_voltage=5;
float gpio8_voltage=5.254654;
float tsref_voltage = 0; // Example: 5V in μV
float rntc=2.56;

extern volatile uint32_t ms_counter ;



//// External functions to access the device
//extern void writeReg(uint8_t bID, uint16_t wAddr, uint8_t data, uint8_t len, uint8_t writeType);
//extern uint8_t readReg(uint8_t bID, uint16_t wAddr, uint8_t* pData, uint8_t len, uint32_t timeout, uint8_t readType);

// Function to enable TSREF
void enableTSREF(void) {
	writeReg(1, BQ79616_CONTROL2, 0x01, 1,FRMWRT_SGL_W);
	writeReg(2, BQ79616_CONTROL2, 0x01, 1,FRMWRT_SGL_W);
	HAL_Delay(10); // Wait 1.35ms for TSREF to stabilize
}

// Function to configure GPIO1 as ADC input
void configureGPIO8_ADC(void) {
	writeReg(1, BQ79616_GPIO_CONF4, 0x10, 1, FRMWRT_SGL_W);
	writeReg(2, BQ79616_GPIO_CONF4, 0x10, 1, FRMWRT_SGL_W);
}

//// Function to start the Main ADC
//void startMainADC(void) {
//    writeReg(0, ADC_CTRL1, MAIN_GO_BIT, 1, FRMWRT_ALL_W);
//}

// Function to read GPIO1 voltage in μV
float readGPIO1Voltage(void) {
	uint8_t hi, lo;
	int16_t raw_value;
	float voltage_uV;

	uint8_t buffer[1];

	hi = readReg(1, GPIO1_HI_REG, buffer, 1, 0, FRMWRT_SGL_R);
	lo = readReg(1, GPIO1_LO_REG, buffer, 1, 0, FRMWRT_SGL_R);

	raw_value = (int16_t)((hi << 8) | lo);
	voltage_uV = raw_value * VLSB_GPIO;

	return voltage_uV;
}

// Function to read TSERF voltage in μV
float readTSREFVoltage(void) {


	float voltage_uV;

	uint8_t buffer[1];

	hi = readReg(1, TSREF_HI, buffer, 1, 0, FRMWRT_SGL_R);
	lo = readReg(1, TSREF_LO, buffer, 1, 0, FRMWRT_SGL_R);

	raw_value = (int16_t)((hi << 8) | lo);
	voltage_uV = raw_value * VLSB_GPIO;

	return voltage_uV;
}



float readGPIO8Voltage(uint8_t BID) {

	float voltage_uV;

	uint16_t buffer[2];

	readReg(BID, GPIO8_HI, &buffer[1], 1, 0, FRMWRT_SGL_R);
	readReg(BID, GPIO8_LO, &buffer[2], 1, 0, FRMWRT_SGL_R);

	//raw_value = (int16_t)((hi << 8) | lo);
	raw_value =((buffer[1] << 8) | buffer[2]);
	voltage_uV = (int16_t)raw_value *VLSB_GPIO/1000000;
	return voltage_uV;
}






/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */
	//BareMetal code

	HAL_TIM_Base_Start_IT(&htim2);

	//=============================
	//Initializing Daisy Chain Communication
	//=============================

	Wake79600();

	Bridge_AutoAddress();

	HAL_Delay(10);
	enableTSREF();
	configureGPIO8_ADC();

	//=============================
	//Initializing Daisy Chain ADCs
	//=============================
	writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_STK_W);
	HAL_Delay(10);
	writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_STK_W);
	HAL_Delay(10);

	//Verifying ADC init
	readReg(1, BQ79616_ADC_CTRL1, &received_data1, 1, 0, FRMWRT_SGL_R);

	readReg(2, BQ79616_ADC_CTRL1, &received_data2, 1, 0, FRMWRT_SGL_R);


	//=============Define Tasks=================//
	xTaskCreate((TaskFunction_t) StartDefaultTask, "defaultTask", 128, NULL,(UBaseType_t) 0, &defaultTaskHandle);

	//=============Start the Scheduler================//
	vTaskStartScheduler();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		HAL_Delay(10);
		final_value = test2();


		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_Delay(200);

		gpio8_voltage = readGPIO8Voltage(1);
		HAL_Delay(1000);
		gpio8_voltage = readGPIO8Voltage(2);


		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 71;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 999;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 1000000;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void StartDefaultTask(void const * argument)
{
	/* USER CODE BEGIN 5 */
	/* Infinite loop */
	for(;;)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		vTaskDelay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		vTaskDelay(1000);
		// osDelay(1);
	}
	/* USER CODE END 5 */
}




/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
