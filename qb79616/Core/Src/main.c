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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "BMS_Config.h"
#include "BMS_tests.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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

//---------------------
//RTOS MACROS
//--------------------
#define NOTIFY_BMS_INIT_DONE   	(1U << 0)
#define NOTIFY_BMS_GOT_MSG		(1U << 1)
#define NOTIFY_BMS_GOT_VOLT		(1U << 1)
#define NOTIFY_BMS_GOT_TEMP		(1U << 2)
#define NOTIFY_BMS_GOT_FS		(1U << 3)



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//-----------------------------------------
//Define Tasks Handler to hold task ID
//-----------------------------------------
//TaskHandle_t defaultTaskHandle;

TaskHandle_t BmsInitTaskHandle;

#ifdef SIMPLETASK

TaskHandle_t BMS_DiagnosticHandle;

#else

TaskHandle_t BmsCellVoltageTaskHandle;
TaskHandle_t BmsReadTempTaskHandle;

#ifdef ADVANCEDTASK

TaskHandle_t BmsCommsTaskHandle;

#elif defined(EVENT_GROUP)

TaskHandle_t BmsMonitorTaskHandle;
TaskHandle_t BmsCommandTaskHandle;
TaskHandle_t BmsFaultTaskHandle;

#endif
#endif

//----------------
//RTOS EVENTGroup Bits
//----------------
const EventBits_t BMS_INIT_DONE_BIT = (1 << 0);

//todo Make a global event bit variable that holds all initialization bits

EventGroupHandle_t BMS_EventGroup;

//----------------
//RTOS Queues
//----------------

QueueHandle_t bmsCmdQueue;
QueueHandle_t bmsVoltageQueue;
QueueHandle_t bmsTempQueue;


//----------------
//RTOS MUTEX
//----------------
#ifdef EVENT_GROUP
SemaphoreHandle_t UART_MUTEX;
#endif

//----------------
//Fault Variables
//----------------
uint8_t fault_summary = 0;

//Private user Variables
uint8_t received_data1 = 0;
uint8_t received_data2 = 0;
uint8_t Buffer[5];

float final_value = 0;
extern int cellVoltages_board[SLAVEBOARDS][16];

int16_t raw_value;
uint8_t hi, lo;

float gpio1_voltage=5;
float gpio8_voltage=5.254654;
float tsref_voltage = 0; // Example: 5V in μV
float rntc=2.56;

extern volatile uint32_t ms_counter ;




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

//Declare Tasks Entery point
//void StartDefaultTask(void const * argument);

void BMS_Init(void const * argument);

#ifdef SIMPLETASK

void BMS_Diagnostic(void const * argument);

#else
#ifdef ADVANCETASK

void BMS_CommsTask(void const * argument);

#elif defined(EVENT_GROUP)
//A Task that periodically pull Cell voltage and Temperature reading
void BMS_MonitorTask(void const * argument);
//Receives Command in a Queue and Sends Commands to Daisy chains on demand
void BMS_CommadTask(void const * argument);
#endif

//Sends a read voltage cmd to BMS queue to read cell voltages
void BMS_CellVoltageTask(void const * argument);
//Sends a read voltage cmd to BMS queue to read cell temp
void BMS_ReadTempTask(void const * argument);

//Activated by an Event group to trigger shutdown circuit and Handle fault
void BMS_FaultTask(void const * argument);

#endif
//todo Can Task


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


//// External functions to access the device
//extern void writeReg(uint8_t bID, uint16_t wAddr, uint8_t data, uint8_t len, uint8_t writeType);
//extern uint8_t readReg(uint8_t bID, uint16_t wAddr, uint8_t* pData, uint8_t len, uint32_t timeout, uint8_t readType);

// Function to enable TSREF
void enableTSREF(void) {
	writeReg(1, BQ79616_CONTROL2, 0x01, 1,FRMWRT_SGL_W);
	writeReg(2, BQ79616_CONTROL2, 0x01, 1,FRMWRT_SGL_W);
	vTaskDelay(10); // Wait 1.35ms for TSREF to stabilize
}

// Function to configure GPIO1 as ADC input
void configureGPIO8_ADC(void) {
	writeReg(1, BQ79616_GPIO_CONF4, 0x10, 1, FRMWRT_SGL_W);
	writeReg(2, BQ79616_GPIO_CONF4, 0x10, 1, FRMWRT_SGL_W);
}


// Function to read TSERF voltage in μV  !!!!!!!! Incorrect Function !!!!
float readTSREFVoltage(void) {


	float voltage_uV;

	uint8_t buffer[1];

	hi = readReg(1, TSREF_HI, buffer, 1, 0, FRMWRT_SGL_R);
	lo = readReg(1, TSREF_LO, buffer, 1, 0, FRMWRT_SGL_R);

	raw_value = (int16_t)((hi << 8) | lo);
	voltage_uV = raw_value * VLSB_GPIO;

	return voltage_uV;
}

float readGPIOVoltage(uint8_t BID, uint8_t GPIO_NUM) {

	float voltage_uV = 989; //Special value indicating invalid GPIO_NUM
	uint16_t buffer[2];
	if((GPIO_NUM >= 1) && (GPIO_NUM <= 8))
	{

		readReg(BID, (GPIO1_HI + 2*(GPIO_NUM - 1)), (uint8_t*)(&buffer[0]), 1, 0, FRMWRT_SGL_R);
		readReg(BID, (GPIO1_LO + 2*(GPIO_NUM - 1)), (uint8_t*)(&buffer[1]), 1, 0, FRMWRT_SGL_R);

		//raw_value = (int16_t)((hi << 8) | lo);
		raw_value =((buffer[1] << 8) | buffer[2]);
		voltage_uV = (int16_t)raw_value *VLSB_GPIO/1000000;
	}
	return voltage_uV;
}

extern EventGroupHandle_t uartEventGroup;
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

	HAL_TIM_Base_Start(&htim2);

	//==================Define EventGroups===================//
	BMS_EventGroup = xEventGroupCreate();
	if (BMS_EventGroup == NULL)
	{
		/* Handle error: insufficient heap */
		while(1);
	}
	/* Create Event Group */
		uartEventGroup = xEventGroupCreate();

	//==================Define Queues===================//

	bmsCmdQueue = xQueueCreate(5, sizeof(BMS_Request_t));
	bmsVoltageQueue = xQueueCreate(3, sizeof(float));
	bmsTempQueue = xQueueCreate(3, sizeof(float));


	//==================Define MUTEX===================//
#ifdef EVENT_GROUP
	UART_MUTEX = xSemaphoreCreateMutex();
#endif

	//==================Define Tasks===================//
	//xTaskCreate((TaskFunction_t) StartDefaultTask, "defaultTask", 128, NULL,(UBaseType_t) 0, &defaultTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_Init, "BMS_Init", 128, NULL,(UBaseType_t) 5, &BmsInitTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_ReadTempTask, "BMS_Read_Temp_Task", 256, NULL,(UBaseType_t) 2, &BmsReadTempTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CellVoltageTask, "BMS_CellVoltage_Task", 256, NULL,(UBaseType_t) 2, &BmsCellVoltageTaskHandle);
#ifdef EVENT_GROUP
	xTaskCreate((TaskFunction_t) BMS_MonitorTask, "BMS_MonitorTask", 256, NULL,(UBaseType_t) 3, &BmsMonitorTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CommadTask, "BMS_CommadTask", 128, NULL,(UBaseType_t) 3, &BmsCommandTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_FaultTask, "BMS_FaultTask", 128, NULL,(UBaseType_t) 4, &BmsFaultTaskHandle);
#elif defined(ADVANCETASK)
	xTaskCreate((TaskFunction_t) BMS_CommsTask, "BMS_Comms_Task", 256, NULL,(UBaseType_t) 3, &BmsCommsTaskHandle);
#elif defined(SIMPLETASK)
	xTaskCreate((TaskFunction_t) BMS_Diagnostic, "BMS_Diagnostic", 512, NULL,(UBaseType_t) 3, &BMS_DiagnosticHandle);
#endif


	//=============Start the Scheduler================//
	vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		//		HAL_Delay(10);
		//		final_value = test2();
		//
		//
		//		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		//		HAL_Delay(200);
		//		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		//		HAL_Delay(200);
		//
		//		gpio8_voltage = readGPIOVoltage(1, 8);
		//		HAL_Delay(1000);
		//		gpio8_voltage = readGPIOVoltage(2, 8);


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
  htim2.Init.Period = 0xffff;
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
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

<<<<<<< HEAD
	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
>>>>>>> Fault_2
}

/* USER CODE BEGIN 4 */

void StartDefaultTask(void const * argument)
{
	/* USER CODE BEGIN 5 */
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

void BMS_Init(void const * argument)
{
	//=============================
	//Initializing Daisy Chain Communication
	//=============================

	Wake79600_RTOS();

	Bridge_AutoAddress();

	vTaskDelay(10);
	enableTSREF();
	configureGPIO8_ADC();

	//=============================
	//Initializing Daisy Chain ADCs
	//=============================
	writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_STK_W);
	vTaskDelay(10);
	writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_STK_W);
	vTaskDelay(10);

	//Verifying ADC init
	readReg(1, BQ79616_ADC_CTRL1, &received_data1, 1, 0, FRMWRT_SGL_R);

	readReg(2, BQ79616_ADC_CTRL1, &received_data2, 1, 0, FRMWRT_SGL_R);

#ifdef SIMPLETASK

	xTaskNotify(BMS_DiagnosticHandle, NOTIFY_BMS_INIT_DONE, eSetBits);

#elif defined(ADVANCETASK)

	xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_INIT_DONE, eSetBits);
	xTaskNotify(BmsReadTempTaskHandle, NOTIFY_BMS_INIT_DONE, eSetBits);
	xTaskNotify(BmsCommsTaskHandle, NOTIFY_BMS_INIT_DONE, eSetBits);

#elif defined(EVENT_GROUP)
	xEventGroupSetBits(BMS_EventGroup, BMS_INIT_DONE_BIT);
#endif

	vTaskDelete(NULL);

}

void BMS_Diagnostic(void const * argument)
{
	xTaskNotifyWait(0, NOTIFY_BMS_INIT_DONE, NULL, portMAX_DELAY);
	for(;;)
	{
		final_value = test2();

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		vTaskDelay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		vTaskDelay(100);

		for(uint8_t i = 1; i <= SLAVEBOARDS; i++)
		{
			gpio8_voltage = readGPIOVoltage(i, 8);
			vTaskDelay(50);
		}
		vTaskDelay(50);
	}


}

void BMS_CommsTask(void const * argument)
{
	float buffer = 0;
	BMS_Request_t req;
	xTaskNotifyWait(0, NOTIFY_BMS_INIT_DONE, NULL, portMAX_DELAY);
	for(;;)
	{
		if(xQueueReceive(bmsCmdQueue, &req, portMAX_DELAY) == pdTRUE)
		{
			switch(req.cmd)
			{
			case CMD_READ_CELL_VOLTAGES:
				buffer = test2();
				xQueueSendToBack(bmsVoltageQueue, &buffer, (TickType_t)10);
				break;

			case CMD_READ_GPIO_ADC:
				//todo make it read all the boards temps
				buffer = readGPIOVoltage(req.BOARD_NUM, req.GPIO_NUM);
				xQueueSendToBack(bmsTempQueue, &buffer, (TickType_t)10);
				break;
			}
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			vTaskDelay(50);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
			vTaskDelay(50);

			xTaskNotify(req.requester, NOTIFY_BMS_GOT_MSG, eSetBits);
		}
	}

}

//Sends a read voltage cmd to BMS queue to read cell voltages
void BMS_CellVoltageTask(void const * argument)
{
	BMS_Request_t req;
	req.cmd = CMD_READ_CELL_VOLTAGES;
	req.requester = xTaskGetCurrentTaskHandle();

#ifdef ADVANCETASK
	xTaskNotifyWait(0, NOTIFY_BMS_INIT_DONE, NULL, portMAX_DELAY);
#elif defined(EVENT_GROUP)
	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
#endif

	for(;;)
	{

#ifdef ADVANCETASK
		xQueueSendToBack(bmsCmdQueue, &req, (TickType_t)10);
#endif
		//wait for MSG to arrive
		//todo Optimize wait delay to be dynamic to the number of slaves
		xTaskNotifyWait(0, NOTIFY_BMS_GOT_VOLT, NULL, pdMS_TO_TICKS(portMAX_DELAY));

		if(xQueueReceive(bmsVoltageQueue, &final_value, (TickType_t)10) == pdTRUE)
	{
			HAL_UART_Transmit(&huart2, (uint8_t*)"Received: ", 10, HAL_MAX_DELAY);

			char numBuf[12];
			itoa((uint32_t)final_value, numBuf, 10);

			HAL_UART_Transmit(&huart2, (uint8_t*)numBuf, strlen(numBuf), HAL_MAX_DELAY);
			HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
		}
		else
		{
			//indicate message is not received
			HAL_UART_Transmit(&huart2, (uint8_t*)"not Received", 12, HAL_MAX_DELAY);
		}

	}
}

//Sends a read voltage cmd to BMS queue to read cell temp
void BMS_ReadTempTask(void const * argument)
{
	BMS_Request_t req;
	req.cmd = CMD_READ_GPIO_ADC;
	req.requester = xTaskGetCurrentTaskHandle();
	//todo make it GPIO_generic
	req.BOARD_NUM = 1;
	req.GPIO_NUM = 8;

#ifdef ADVANCETASK
	xTaskNotifyWait(0, NOTIFY_BMS_INIT_DONE, NULL, portMAX_DELAY);
#elif defined(EVENT_GROUP)
	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
#endif

	for(;;)
	{
#ifdef ADVANCETASK
		xQueueSendToBack(bmsCmdQueue, &req, (TickType_t)10);
#endif

		//todo Optimize wait delay to be dynamic to the number of slaves
		//wait for MSG to arrive
		xTaskNotifyWait(0, NOTIFY_BMS_GOT_TEMP, NULL, pdMS_TO_TICKS(portMAX_DELAY));

		if(xQueueReceive(bmsTempQueue, &gpio8_voltage, (TickType_t)10) == pdTRUE)
		{
			//process Voltage reading
		}
		else
		{
			//indicate message is not received
		}

	}
}

//A Task that periodically pull Cell voltage and Temperature reading
void BMS_MonitorTask(void const * argument)
{
	float buffer = 0;
	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	for(;;)
	{
		xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
		buffer = test2();
		xQueueSendToBack(bmsVoltageQueue, &buffer, (TickType_t)10);
		xSemaphoreGive(UART_MUTEX);
		xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);
		vTaskDelay(pdMS_TO_TICKS(10)); //delay for Commandtask to take mutex

		xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
		for(uint8_t i = 1; i <= SLAVEBOARDS; i++)
		{
			buffer = readGPIOVoltage(i, Thermistor_GPIO);
			xQueueSendToBack(bmsTempQueue, &buffer, (TickType_t)10);
			vTaskDelay(pdMS_TO_TICKS(5));
		}
		xTaskNotify(BmsReadTempTaskHandle, NOTIFY_BMS_GOT_TEMP, eSetBits);
		xSemaphoreGive(UART_MUTEX);

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		vTaskDelay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		vTaskDelay(100);
	}
}
//Receives Command in a Queue and Sends Commands to Daisy chains on demand
void BMS_CommadTask(void const * argument)
{
	BMS_Request_t req;
	uint8_t cmd_res = 0;
	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	for(;;)
	{
		if(xQueueReceive(bmsCmdQueue, &req, portMAX_DELAY) == pdTRUE)
		{
			xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
			switch(req.cmd)
			{
				case CMD_READ_BRIDGE_FS:
					//fault_summary = Send to bridge to read fault summary
					xTaskNotify(req.requester, NOTIFY_BMS_GOT_FS, eSetBits);
					break;

				case CMD_READ_GPIO_ADC:


					break;
				case CMD_READ_CELL_VOLTAGES:
					break;
			}
			xSemaphoreGive(UART_MUTEX);
			xTaskNotify(req.requester, cmd_res, eSetBits);
		}
	}
}

void BMS_FaultTask(void const * argument)
{
	BMS_Request_t req;
	req.requester = xTaskGetCurrentTaskHandle();
	req.cmd = CMD_READ_BRIDGE_FS;
	uint8_t cmd_res = 0;
	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	for(;;)
	{
		xQueueSendToBack(bmsCmdQueue, &req, (TickType_t)10);
		xTaskNotifyWait(0, NOTIFY_BMS_GOT_FS, NULL, pdMS_TO_TICKS(portMAX_DELAY));
		//read fault_summary variable and handle it
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}



/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
