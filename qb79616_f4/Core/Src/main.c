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
 * If no LICENSE file comes with this software, it is provided AS-IS..
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
#include <stdlib.h>
#include <string.h>
#include "BMS_Config.h"
#include "BMS_tests.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "GUI.h"
#include "Faults.h"
#include "Voltages.h"
#include "Temperatures.h"
#include "Balancing.h"
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
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CAN_HandleTypeDef hcan1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
//-----------------------------------------
// External Fault Variables
//-----------------------------------------
extern uint8_t bridge_faultSummary;

extern uint8_t bridge_faultPWR;
extern uint8_t bridge_faultSYS;
extern uint8_t bridge_faultREG;
extern uint8_t bridge_faultCOMM1;
extern uint8_t bridge_faultCOMM2;



//-----------------------------------------
//Define Tasks Handler to hold task ID
//-----------------------------------------
//TaskHandle_t defaultTaskHandle;

TaskHandle_t BmsInitTaskHandle;
TaskHandle_t BmsCanTxTaskHandle;
TaskHandle_t BmsMonitorTaskHandle;
TaskHandle_t BmsCommandTaskHandle;
TaskHandle_t BmsCellVoltageTaskHandle;
TaskHandle_t BmsReadTempTaskHandle;
TaskHandle_t BmsFaultTaskHandle;
TaskHandle_t BmsBalancingTaskHandle;
TaskHandle_t BmsGUITaskHandle;
TaskHandle_t BmsChargerTaskHandle;
TaskHandle_t BmsCurrentSensorTaskHandle;

//----------------
//RTOS EVENTGroup Bits
//----------------
const EventBits_t BMS_INIT_DONE_BIT = (1 << 0);

//todo Make a global event bit variable that holds all initialization bits

EventGroupHandle_t BMS_EventGroup;

//----------------
//RTOS Queues
//----------------

QueueHandle_t bmsCanTXQueue;
QueueHandle_t bmsCmdQueue;
//QueueHandle_t bmsMeasurmentsQueue;
QueueHandle_t bmsVoltageQueue;
QueueHandle_t bmsTempQueue;


//----------------
//RTOS MUTEX
//----------------
#ifdef EVENT_GROUP
SemaphoreHandle_t UART_MUTEX;
#endif

SemaphoreHandle_t CAN_MUTEX;

//Private user Variables
uint8_t received_data = 0;
uint8_t Buffer[5];

uint8_t init_done = 0;

float battery_volt = 0;




UBaseType_t uxHighWaterMark;
extern EventGroupHandle_t uartEventGroup;

//------------------------
//    CAN Variables
//-----------------------
CAN_TxHeaderTypeDef BMS_CAN_TxHandler;
CAN_TxHeaderTypeDef BMS_CHAR_CAN_TxHandler;
CAN_RxHeaderTypeDef BMS_CAN_RxHandler;

uint32_t TxMailbox;
uint8_t BMS_CAN_TxData[8];
BMS_CAN_Frame_t BMS_CAN_RxData;

//------------------------
//    CHARGER Variables
//-----------------------
volatile uint8_t charger_fault = 0;
volatile TickType_t charger_last_rx_time;


//------------------------
//    Current Sensor Variables
//-----------------------
volatile uint16_t current_buffer[NUM_SAMPLES * NUM_CHANNELS];
uint32_t raw_adc_Vout;
uint32_t raw_adc_Vref;
double current_read;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_UART4_Init(void);
static void MX_CAN1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

//Declare Tasks Entery point
//void StartDefaultTask(void const * argument);

void BMS_Init(void const * argument);

//A Task that periodically pull Cell voltage and Temperature reading
void BMS_MonitorTask(void const * argument);
//Receives Command in a Queue and Sends Commands to Daisy chains on demand
void BMS_CommadTask(void const * argument);
//Sends a read voltage cmd to BMS queue to read cell voltages
void BMS_CellVoltageTask(void const * argument);
//Sends a read voltage cmd to BMS queue to read cell temp
void BMS_ReadTempTask(void const * argument);

//Activated by an Event group to trigger shutdown circuit and Handle fault
void BMS_FaultTask(void const * argument);

void BMS_BalancingTask(void const * argument);

void BMS_CanTX_Task(void const * argument);

void BMS_ChargerTask(void const * argument);

void BMS_GUI_Task(void const * argument);

void BMS_CurrentSensorTask(void const * argument);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//function that converts the data according to the type
//Funtion outputs the value x100 to take only 2 numbers after the decimal point
int16_t BMS_data_convert(BMS_DataType_t type, int data)
{
	switch(type)
	{
	case BMS_DATA_VOLTAGE:
		data = (int16_t)((float)(data) * VOLT_CONV * 100);
		break;
	case BMS_DATA_TEMPERATURE:
		data = (int16_t)(((float)(data) * VLSB_GPIO) * 100);
		break;
	case BMS_CHARGER:
		data = (int16_t)((float)(data) / 10);
		break;
	}

	return data;
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_UART4_Init();
  MX_CAN1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

	BMS_Can_Init();

	//==================Define EventGroups===================//
	BMS_EventGroup = xEventGroupCreate();
	if (BMS_EventGroup == NULL)
	{
		/* Handle error: insufficient heap */
		while(1);
	}
	/* Create Event Group */
	uartEventGroup = xEventGroupCreate();
	faultEventGroup = xEventGroupCreate();
	//==================Define Queues===================//

	bmsVoltageQueue = xQueueCreate(MAX(1, SLAVEBOARDS * 7), sizeof(BMS_Queue_Measurement_t));
	bmsCmdQueue = xQueueCreate(5, sizeof(BMS_Request_t));
	bmsTempQueue = xQueueCreate(MAX(1, SLAVEBOARDS * 7), sizeof(BMS_Queue_Measurement_t));
	bmsCanTXQueue = xQueueCreate(10, sizeof(BMS_CAN_Queue_Message_t));


	//==================Define MUTEX===================//
	UART_MUTEX = xSemaphoreCreateMutex();
	CAN_MUTEX  = xSemaphoreCreateMutex();

	//==================Define Tasks===================//
	//xTaskCreate((TaskFunction_t) StartDefaultTask, "defaultTask", 128, NULL,(UBaseType_t) 0, &defaultTaskHandle);
//	xTaskCreate((TaskFunction_t) BMS_Init, "BMS_Init", 256, NULL,(UBaseType_t) 5, &BmsInitTaskHandle);
//	xTaskCreate((TaskFunction_t) BMS_CanTX_Task, "BMS_CanTX_Task", 128, NULL,(UBaseType_t) 5, &BmsCanTxTaskHandle);
//	xTaskCreate((TaskFunction_t) BMS_MonitorTask, "BMS_MonitorTask", 128, NULL,(UBaseType_t) 3, &BmsMonitorTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CommadTask, "BMS_CommadTask", 256, NULL,(UBaseType_t) 3, &BmsCommandTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_ReadTempTask, "BMS_Read_Temp_Task", 256, NULL,(UBaseType_t) 2, &BmsReadTempTaskHandle);
//	xTaskCreate((TaskFunction_t) BMS_CellVoltageTask, "BMS_CellVoltage_Task", 256, NULL,(UBaseType_t) 2, &BmsCellVoltageTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_FaultTask, "BMS_FaultTask", 256, NULL,(UBaseType_t) 4, &BmsFaultTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_BalancingTask, "BMS_BalancingTask", 256, NULL,(UBaseType_t) 4, &BmsBalancingTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_ChargerTask, "BMS_ChargerTask", 256, NULL,(UBaseType_t) 3, &BmsChargerTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CurrentSensorTask, "CurrentSensor_Task", 128, NULL,(UBaseType_t) 1, &BmsCurrentSensorTaskHandle);

#ifdef GUI
	xTaskCreate((TaskFunction_t) BMS_GUI_Task, "BMS_GUITask", 128, NULL,(UBaseType_t) 2, &BmsGUITaskHandle);
#endif


	//=============Start the Scheduler================//
	vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{


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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 5;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_14TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void StartDefaultTask(void const * argument)
{
	/* USER CODE BEGIN 5 */
	for(;;)
	{

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

#ifdef CHAIN
	Bridge_AutoAddress();
#elif defined(RING)
	AutoAddress_Ring();
	init_done = 1;
#endif

	//todo make it a for loop on the number of slaves
	for(uint8_t i = 1; i < TOTALBOARDS; i++)
	{

		if(configure_OVUV(i, 16) != 1)
		{
			//error
			while(1);
		}
		if(configure_OTUT(i, 0) != 1)
		{
			//error
			while(1);
		}
	}

	Bridge_FaultInit();
	Stack_FaultInit();

	vTaskDelay(10);

	//=============================
	//Initializing Daisy Chain ADCs
	//=============================
	writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_STK_W);
	vTaskDelay(10);

	uint8_t dev_stat;
	for(uint8_t i = 1; i < TOTALBOARDS; i++)
	{
		readReg(i, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
		if((dev_stat& 0x01) == 0){
			//error
			while(1);
		}
	}

	xEventGroupSetBits(BMS_EventGroup, BMS_INIT_DONE_BIT);

	vTaskDelete(NULL);

}


#ifdef GUI
void BMS_GUI_Task(void const * argument)
{
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	for(;;)
	{

		Send_GUI_Reading();

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		vTaskDelay(500);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		vTaskDelay(500);
	}
}

#endif
//A Task that periodically pull Cell voltage and Temperature reading
void BMS_MonitorTask(void const * argument)
{

	BMS_Queue_Measurement_t buffer;
	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	for(;;)
	{
		int raw_slave_volt = 0;
		int raw_battary_voltage = 0;

		xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
		Bridge_CheckFaults();
		Stack_CheckFaultSummary();
		//Send_GUI_Reading();
		{
			//buffer = test2();
			//buffer.type = BMS_DATA_VOLTAGE;

			//Fetching the Voltage Value of Each Slave
			for(uint8_t i = 1; i <= SLAVEBOARDS; i++)
			{
				raw_slave_volt = 0;
				if(readSlaveVoltage(i, ACTIVE_CELLS, &raw_slave_volt) != BMS_OK)
				{
					//error
				}
				raw_battary_voltage += raw_slave_volt;

				//adding Voltage reading in queue

				buffer.slave_id = i;
				buffer.value = BMS_data_convert(BMS_DATA_VOLTAGE, raw_slave_volt);
				xQueueSendToBack(bmsVoltageQueue, &buffer, (TickType_t)10);
				//				xQueueSendToBack(bmsMeasurmentsQueue, &buffer, (TickType_t)10);
				vTaskDelay(pdMS_TO_TICKS(5));
				xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);
			}

			//calculating Battery voltage
			buffer.slave_id = BATTERYVOLT_ID;
			buffer.value = BMS_data_convert(BMS_DATA_VOLTAGE, raw_battary_voltage);
			xQueueSendToBack(bmsVoltageQueue, &buffer, (TickType_t)10);
			//			xQueueSendToBack(bmsMeasurmentsQueue, &buffer, (TickType_t)10);
			xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);
		}
		xSemaphoreGive(UART_MUTEX);
		xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);

		vTaskDelay(pdMS_TO_TICKS(10)); //delay for Commandtask to take mutex

		xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
		{
			//			buffer.type = BMS_DATA_TEMPERATURE;
			for(uint8_t i = 1; i <= SLAVEBOARDS; i++)
			{
				buffer.slave_id = i;
				uint32_t temperautre_sum = 0;
				uint8_t j;
				for(j = 1; j <= 4; j++)
				{
					uint16_t temperature_temp = readGPIOVoltage(i, j, &(GpioReadings[i-1][j-1]));
					temperautre_sum += temperature_temp;

				}
				buffer.value = BMS_data_convert(BMS_DATA_TEMPERATURE, temperautre_sum/j);
				xQueueSendToBack(bmsTempQueue, &buffer, (TickType_t)10);
				vTaskDelay(pdMS_TO_TICKS(5));
				xTaskNotify(BmsReadTempTaskHandle, NOTIFY_BMS_GOT_TEMP, eSetBits);
			}
		}

		xSemaphoreGive(UART_MUTEX);

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
				Bridge_CheckFaults();
				Stack_CheckFaultSummary();
				//				Send_GUI_Reading ();
				cmd_res = 0;
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

//Sends a read voltage cmd to BMS queue to read cell voltages
void BMS_CellVoltageTask(void const * argument)
{
	BMS_Request_t req;
	req.cmd = CMD_READ_CELL_VOLTAGES;
	req.requester = xTaskGetCurrentTaskHandle();
	BMS_Queue_Measurement_t volt_buffer;
	BMS_CAN_Queue_Message_t can_buffer;
	can_buffer.type = BMS_DATA_VOLTAGE;
	memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));

	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

	for(;;)
	{


		if(xQueueReceive(bmsVoltageQueue, &volt_buffer, (TickType_t)10) == pdTRUE)
		{

			if(volt_buffer.slave_id == BATTERYVOLT_ID)
			{
				battery_volt = ((float)(volt_buffer.value) / 100);
			}

			can_buffer.Data.frame.slave[(volt_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = volt_buffer.value;

			if(((volt_buffer.slave_id % CAN_DATA_PER_FRAME) == 0))
			{
				can_buffer.first_slave_id = volt_buffer.slave_id - CAN_DATA_PER_FRAME + 1;
				xQueueSendToBack(bmsCanTXQueue, &can_buffer, (TickType_t)10);
				memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
			}

		}
		else
		{
			//todo Optimize wait delay to be dynamic to the number of slaves
			xTaskNotifyWait(0, NOTIFY_BMS_GOT_VOLT, NULL, portMAX_DELAY);
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

	BMS_Queue_Measurement_t temperature_buffer;
	BMS_CAN_Queue_Message_t can_buffer;
	can_buffer.type = BMS_DATA_TEMPERATURE;
	memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
	uint8_t prev_first_ID = 0;

	//todo Make timeout and Handling to this timeout
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

	for(;;)
	{

		//todo Optimize wait delay to be dynamic to the number of slaves
		//wait for MSG to arrive
		xTaskNotifyWait(0, NOTIFY_BMS_GOT_TEMP, NULL, pdMS_TO_TICKS(portMAX_DELAY));

		if(xQueueReceive(bmsTempQueue, &temperature_buffer, (TickType_t)10) == pdTRUE)
		{

			if((temperature_buffer.slave_id % CAN_DATA_PER_FRAME == 1) &&  (temperature_buffer.slave_id != prev_first_ID))
			{
				if(prev_first_ID != 0)
				{
					can_buffer.first_slave_id = temperature_buffer.slave_id - CAN_DATA_PER_FRAME + 1;
					xQueueSendToBack(bmsCanTXQueue, &can_buffer, (TickType_t)10);
					memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
				}
				can_buffer.Data.frame.slave[(temperature_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = temperature_buffer.value;
				prev_first_ID = temperature_buffer.slave_id;
			}
			else if(temperature_buffer.slave_id % CAN_DATA_PER_FRAME != 1)
			{
				can_buffer.Data.frame.slave[(temperature_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = temperature_buffer.value;
			}
			else if((temperature_buffer.slave_id % CAN_DATA_PER_FRAME == 1) && (temperature_buffer.slave_id == prev_first_ID))
			{
				can_buffer.first_slave_id = prev_first_ID;
				xQueueSendToBack(bmsCanTXQueue, &can_buffer, (TickType_t)10);
				memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
				can_buffer.Data.frame.slave[(temperature_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = temperature_buffer.value;
				prev_first_ID = temperature_buffer.slave_id;
			}

		}
		else
		{
			//indicate message is not received
			//todo Optimize wait delay to be dynamic to the number of slaves
			xTaskNotifyWait(0, NOTIFY_BMS_GOT_TEMP, NULL, portMAX_DELAY);
		}

	}
}


void BMS_CanTX_Task(void const * argument)
{
	BMS_CAN_Queue_Message_t CAN_Queue_Buffer;
	BMS_CAN_TxHandler.RTR = CAN_RTR_DATA;           //Set RTR bit to 0(sends data)
	BMS_CAN_TxHandler.DLC = CAN_MSG_SIZE;           //Data sent is CAN_MSG_SIZE bytes in BMS_Config.h


	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

	for(;;)
	{
		if(xQueueReceive(bmsCanTXQueue, &CAN_Queue_Buffer, (TickType_t)10) == pdTRUE)
		{
			//todo SET proper IDS
			if(CAN_Queue_Buffer.type == BMS_DATA_VOLTAGE)
			{
				BMS_CAN_TxHandler.StdId = CAN_VOLT_ID + ((CAN_Queue_Buffer.first_slave_id - 1) / CAN_DATA_PER_FRAME);                //Message ID
				BMS_CAN_TxHandler.ExtId = 0x00;                 //Message ID extension for CAN extend
				BMS_CAN_TxHandler.IDE = CAN_ID_STD;
			}
			else if(CAN_Queue_Buffer.type == BMS_DATA_TEMPERATURE)
			{
				BMS_CAN_TxHandler.StdId = CAN_TEMP_ID + ((CAN_Queue_Buffer.first_slave_id - 1) / CAN_DATA_PER_FRAME);                //Message ID
				BMS_CAN_TxHandler.ExtId = 0x00;                 //Message ID extension for CAN extend
				BMS_CAN_TxHandler.IDE = CAN_ID_STD;
			}
			else if(CAN_Queue_Buffer.type == BMS_CHARGER)
			{
				BMS_CAN_TxHandler.StdId = 0x00;               //Message ID
				BMS_CAN_TxHandler.ExtId = CAN_BMS_TO_CH;      //Message ID extension for CAN extend
				BMS_CAN_TxHandler.IDE = CAN_ID_EXT;             //CAN ID is 11 bits
			}

			xSemaphoreTake(CAN_MUTEX, portMAX_DELAY);
			if( HAL_CAN_AddTxMessage(&hcan1, &BMS_CAN_TxHandler, (uint8_t*)&(CAN_Queue_Buffer.Data), &TxMailbox) != HAL_OK)
			{
				//error
				HAL_UART_Transmit(&huart4, (uint8_t*)"CAN Failed", 10, HAL_MAX_DELAY);
			}
			xSemaphoreGive(CAN_MUTEX);
			vTaskDelay(pdMS_TO_TICKS(10)); //delay for Charger task to take mutex

			//			vTaskDelay(pdMS_TO_TICKS(50));
		}
	}

}
BMS_CAN_Frame_t tx_frame = {0};

void BMS_ChargerTask(void const * argument)
{


	BMS_CHAR_CAN_TxHandler.RTR = CAN_RTR_DATA;           //Set RTR bit to 0(sends data)
	BMS_CHAR_CAN_TxHandler.DLC = CAN_MSG_SIZE;           //Data sent is CAN_MSG_SIZE bytes in BMS_Config.h
	BMS_CHAR_CAN_TxHandler.StdId = 0x00;               //Message ID
	BMS_CHAR_CAN_TxHandler.ExtId = CAN_BMS_TO_CH;      //Message ID extension for CAN extend
	BMS_CHAR_CAN_TxHandler.IDE = CAN_ID_EXT;             //CAN ID is 11 bits
//	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

	int i = 0;
	while(1)
	{

		//todo Wait for EXTI or current sensor outputs zero
		while(i != 0)
		{

		}
		charger_fault = 0;
		i++;

		while(1)
		{

//			Check charger communication timeout
			if((xTaskGetTickCount() - charger_last_rx_time) > pdMS_TO_TICKS(5000))
			{
				charger_fault = 1;
			}

			//Charger fault detected
			memset(&tx_frame,0,sizeof(tx_frame));

			if(charger_fault)
			{

				tx_frame.bytes[0] = 0;
				tx_frame.bytes[1] = 0;
				tx_frame.bytes[2] = 0;
				tx_frame.bytes[3] = 0;
				tx_frame.bytes[4] = CHAR_STOP;

				//send stop frame
				xSemaphoreTake(CAN_MUTEX, portMAX_DELAY);
				if( HAL_CAN_AddTxMessage(&hcan1, &BMS_CHAR_CAN_TxHandler, (uint8_t*)&(tx_frame), &TxMailbox) != HAL_OK)
				{
					//error
					HAL_UART_Transmit(&huart4, (uint8_t*)"CAN Failed", 10, HAL_MAX_DELAY);
				}
				xSemaphoreGive(CAN_MUTEX);//
				vTaskDelay(10);
				break;
			}
			else
			{
				tx_frame.bytes[0] = CHAR_MAX_VOLT_High;
				tx_frame.bytes[1] = CHAR_MAX_VOLT_low;
				tx_frame.bytes[2] = CHAR_MAX_AMP_High;
				tx_frame.bytes[3] = CHAR_MAX_AMP_low;
				tx_frame.bytes[4] = CHAR_START;
				xSemaphoreTake(CAN_MUTEX, portMAX_DELAY);
				if( HAL_CAN_AddTxMessage(&hcan1, &BMS_CHAR_CAN_TxHandler, (uint8_t*)&(tx_frame), &TxMailbox) != HAL_OK)
				{
					//error
					HAL_UART_Transmit(&huart4, (uint8_t*)"CAN Failed", 10, HAL_MAX_DELAY);
				}
				xSemaphoreGive(CAN_MUTEX);
				vTaskDelay(10);
			}

			vTaskDelay(pdMS_TO_TICKS(1000));

		}

	}
}



void BMS_FaultTask(void const * argument)
{
	while(1)
	{
		xEventGroupWaitBits(
				faultEventGroup,
				FAULT_EVENT_BIT,
				pdTRUE,
				pdFALSE,
				portMAX_DELAY
		);

		//		Bridge_CheckFaults();
		//		Stack_CheckFaultSummary();
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		vTaskDelay(1000);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
		vTaskDelay(1000);
		Send_GUI_Reading ();

	}

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

void BMS_BalancingTask(void const * argument)
{
	// waits for BMS_Init to finish
	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

	// one-time setup
	xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
	balancing_init();
	xSemaphoreGive(UART_MUTEX);

	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(BAL_LOOP_DELAY_MS));  // 60 second delay

		xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
		balancing_update();
		xSemaphoreGive(UART_MUTEX);
	}
}


/**================================================================
 * @Fn				- CurrentSensorTask
 * @breif			- Function implementing the CurrentSensor_Task thread
 * 					- Current estimation Using ADC1
 * @param [in]		- argument: Not Used
 * Delay:			- This task is blocked for 10 ticks unless the event is set by the ISR.
 * Note:			- Tick = 10 ms
 */
void BMS_CurrentSensorTask(void const * argument)
{
    double Vref_V = 0;
    double Vout_V = 0;


    while(1)
    {
        // 1. Tell hardware to collect exactly 10 pairs of samples in the background
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)current_buffer, NUM_SAMPLES * NUM_CHANNELS);

        // 2. Sleep indefinitely until the DMA interrupt signals us it is done
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // 3. Hardware has finished! Processing phase
        raw_adc_Vref = 0;
        raw_adc_Vout = 0;

        // The DMA fills the array like this: [Vref0, Vout0, Vref1, Vout1...]
        for(uint8_t i = 0; i < (NUM_SAMPLES * NUM_CHANNELS); i += 2)
        {
            raw_adc_Vref += current_buffer[i];
            raw_adc_Vout += current_buffer[i+1];
        }

        // 4. Calculate final values
        Vref_V = (double)(((raw_adc_Vref )/40950.0)*3.3);
        Vout_V = (double)(((raw_adc_Vout )/40950.0)*3.3);
        current_read = (Vout_V - Vref_V) * (HASS_50_S_IPN/0.625);

        // 5. Rest for 20ms before starting the next batch
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	//Not check on ID, As Can filter only receives charger frames
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &BMS_CAN_RxHandler, BMS_CAN_RxData.bytes) == HAL_OK)
	{
		// Charger broadcast received
		charger_last_rx_time = xTaskGetTickCountFromISR();

		if((BMS_CAN_RxData.bytes[4] & 0b1000) == 0) //charger is ON
		{

			if((BMS_CAN_RxData.bytes[4] & 0b0111) != 0) //Fault Occurred
			{
//				charger_fault = 1;
			}
		}

		return; // Error
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc1) {
    if (hadc1->Instance == ADC1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Unblock the Current Sensor Task
        vTaskNotifyGiveFromISR(BmsCurrentSensorTaskHandle, &xHigherPriorityTaskWoken);

        // Context switch if needed
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
