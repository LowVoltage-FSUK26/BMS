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
CAN_HandleTypeDef hcan;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

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
TaskHandle_t BmsCanTaskHandle;

#ifdef SIMPLETASK

TaskHandle_t BMS_DiagnosticHandle;

#elif defined(EVENT_GROUP)

TaskHandle_t BmsMonitorTaskHandle;
TaskHandle_t BmsCommandTaskHandle;
TaskHandle_t BmsCellVoltageTaskHandle;
TaskHandle_t BmsReadTempTaskHandle;
TaskHandle_t BmsFaultTaskHandle;

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

QueueHandle_t bmsCanQueue;
QueueHandle_t bmsCmdQueue;
QueueHandle_t bmsMeasurmentsQueue;

//----------------
//RTOS MUTEX
//----------------
#ifdef EVENT_GROUP
SemaphoreHandle_t UART_MUTEX;
#endif


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
CAN_RxHeaderTypeDef BMS_CAN_RxHandler;
uint32_t TxMailbox;
uint8_t BMS_CAN_TxData[8];
uint8_t BMS_CAN_RxData[8];


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CAN_Init(void);
/* USER CODE BEGIN PFP */

//Declare Tasks Entery point
//void StartDefaultTask(void const * argument);

void BMS_Init(void const * argument);

#ifdef SIMPLETASK

void BMS_Diagnostic(void const * argument);

#elif defined(EVENT_GROUP)
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

#endif

void BMS_CanTask(void const * argument);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &BMS_CAN_RxHandler, BMS_CAN_RxData) != HAL_OK)
	{
		return; // Error
	}

}

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
	}

	return data;
}

//void UART_PrintFrame(BMS_CAN_Queue_Message_t *frame)
//{
//    char buffer[100];
//
//    snprintf(buffer, sizeof(buffer),
//        "slave[%d] = %d v slave[%d] = %d v slave[%d] = %d slave[%d] = %d v\r\n",
//		frame->first_slave_id,
//        frame->Data.frame.slave[0],
//		frame->first_slave_id + 1,
//		frame->Data.frame.slave[1],
//		frame->first_slave_id + 2,
//		frame->Data.frame.slave[2],
//		frame->first_slave_id + 3,
//		frame->Data.frame.slave[3]);
//
//    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
//}

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
	MX_CAN_Init();
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

	bmsCmdQueue = xQueueCreate(5, sizeof(BMS_Request_t));
	bmsMeasurmentsQueue = xQueueCreate(SLAVEBOARDS * 7, sizeof(BMS_Queue_Measurement_t));
	bmsCanQueue = xQueueCreate(10, sizeof(BMS_CAN_Queue_Message_t));


	//==================Define MUTEX===================//
#ifdef EVENT_GROUP
	UART_MUTEX = xSemaphoreCreateMutex();
#endif

	//==================Define Tasks===================//
	//xTaskCreate((TaskFunction_t) StartDefaultTask, "defaultTask", 128, NULL,(UBaseType_t) 0, &defaultTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_Init, "BMS_Init", 80, NULL,(UBaseType_t) 5, &BmsInitTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CanTask, "BMS_CanTask", 128, NULL,(UBaseType_t) 5, &BmsCanTaskHandle);
#ifdef SIMPLETASK
	xTaskCreate((TaskFunction_t) BMS_Diagnostic, "BMS_Diagnostic", 128, NULL,(UBaseType_t) 3, &BMS_DiagnosticHandle);
#elif defined(EVENT_GROUP)
	xTaskCreate((TaskFunction_t) BMS_MonitorTask, "BMS_MonitorTask", 128, NULL,(UBaseType_t) 3, &BmsMonitorTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CommadTask, "BMS_CommadTask", 128, NULL,(UBaseType_t) 3, &BmsCommandTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_ReadTempTask, "BMS_Read_Temp_Task", 128, NULL,(UBaseType_t) 2, &BmsReadTempTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_CellVoltageTask, "BMS_CellVoltage_Task", 128, NULL,(UBaseType_t) 2, &BmsCellVoltageTaskHandle);
	xTaskCreate((TaskFunction_t) BMS_FaultTask, "BMS_FaultTask", 80, NULL,(UBaseType_t) 4, &BmsFaultTaskHandle);
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
 * @brief CAN Initialization Function
 * @param None
 * @retval None
 */
static void MX_CAN_Init(void)
{

	/* USER CODE BEGIN CAN_Init 0 */

	/* USER CODE END CAN_Init 0 */

	/* USER CODE BEGIN CAN_Init 1 */

	/* USER CODE END CAN_Init 1 */
	hcan.Instance = CAN1;
	hcan.Init.Prescaler = 18;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_1TQ;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = DISABLE;
	hcan.Init.AutoWakeUp = DISABLE;
	hcan.Init.AutoRetransmission = DISABLE;
	hcan.Init.ReceiveFifoLocked = DISABLE;
	hcan.Init.TransmitFifoPriority = DISABLE;
	if (HAL_CAN_Init(&hcan) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN CAN_Init 2 */

	/* USER CODE END CAN_Init 2 */

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

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_3, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PB12 PB3 */
	GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PA8 */
	GPIO_InitStruct.Pin = GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

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
	readReg(1, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
	if((dev_stat& 0x01) == 0){
		//error
		while(1);
	}
	readReg(2, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
	if((dev_stat& 0x01) == 0){
		//error
		while(1);
	}


#ifdef SIMPLETASK

	xTaskNotify(BMS_DiagnosticHandle, NOTIFY_BMS_INIT_DONE, eSetBits);

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
		battery_volt = readBattaryVoltage();

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		vTaskDelay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		vTaskDelay(100);

		for(uint8_t i = 1; i <= SLAVEBOARDS; i++)
		{
			for(uint8_t j = 1; j <= 4; j++)
			{
				readGPIOVoltage(i, j, &(GpioReadings[i - 1][j - 1]));
				vTaskDelay(50);
			}

		}
		vTaskDelay(50);
	}


}

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
		Send_GUI_Reading();
		{
			//buffer = test2();
			buffer.type = BMS_DATA_VOLTAGE;

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
				//xQueueSendToBack(bmsVoltageQueue, &buffer.value, (TickType_t)10);
				xQueueSendToBack(bmsMeasurmentsQueue, &buffer, (TickType_t)10);

				//if( (i % CAN_DATA_PER_FRAME) == 0)
				xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);
			}

			//calculating Battery voltage
			buffer.slave_id = BATTERYVOLT_ID;
			buffer.value = BMS_data_convert(BMS_DATA_VOLTAGE, raw_battary_voltage);
			//xQueueSendToBack(bmsVoltageQueue, &buffer.value, (TickType_t)10);
			xQueueSendToBack(bmsMeasurmentsQueue, &buffer, (TickType_t)10);
			xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);
		}
		xSemaphoreGive(UART_MUTEX);
		xTaskNotify(BmsCellVoltageTaskHandle, NOTIFY_BMS_GOT_VOLT, eSetBits);

		vTaskDelay(pdMS_TO_TICKS(10)); //delay for Commandtask to take mutex

		xSemaphoreTake(UART_MUTEX, portMAX_DELAY);
		{
			buffer.type = BMS_DATA_TEMPERATURE;
			for(uint8_t i = 1; i <= SLAVEBOARDS; i++)
			{
				buffer.slave_id = i;
				for(uint8_t j = 1; j <= 4; j++)
				{
					float temperature_temp = readGPIOVoltage(i, j, &(GpioReadings[i-1][j-1]));
					buffer.value = (int16_t)(temperature_temp);
					xQueueSendToBack(bmsMeasurmentsQueue, &buffer, (TickType_t)10);
					vTaskDelay(pdMS_TO_TICKS(5));
				}
			}
			xTaskNotify(BmsReadTempTaskHandle, NOTIFY_BMS_GOT_TEMP, eSetBits);
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


		if(xQueueReceive(bmsMeasurmentsQueue, &volt_buffer, (TickType_t)10) == pdTRUE)
		{

			if(volt_buffer.type == BMS_DATA_VOLTAGE)
			{

				if(volt_buffer.slave_id == BATTERYVOLT_ID)
				{
					battery_volt = ((float)(volt_buffer.value) / 100);

					HAL_UART_Transmit(&huart2, (uint8_t*)"Received Voltage: ", 18, HAL_MAX_DELAY);
					char numBuf[12];
					itoa((uint32_t)battery_volt, numBuf, 10);
					HAL_UART_Transmit(&huart2, (uint8_t*)numBuf, strlen(numBuf), HAL_MAX_DELAY);
					HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
				}

				can_buffer.Data.frame.slave[(volt_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = volt_buffer.value;

				if(((volt_buffer.slave_id % CAN_DATA_PER_FRAME) == 0))
				{
					can_buffer.first_slave_id = volt_buffer.slave_id - CAN_DATA_PER_FRAME + 1;
					xQueueSendToBack(bmsCanQueue, &can_buffer, (TickType_t)10);
					memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
				}

			}
			else
			{
				xQueueSendToBack(bmsMeasurmentsQueue, &volt_buffer, (TickType_t)10);
				continue;
			}
		}
		else
		{
			//indicate message is not received
			//HAL_UART_Transmit(&huart2, (uint8_t*)"not Received", 12, HAL_MAX_DELAY);
			//wait for MSG to arrive
			//todo Optimize wait delay to be dynamic to the number of slaves
			xTaskNotifyWait(0, NOTIFY_BMS_GOT_VOLT, NULL, pdMS_TO_TICKS(portMAX_DELAY));
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

		if(xQueueReceive(bmsMeasurmentsQueue, &temperature_buffer, (TickType_t)10) == pdTRUE)
		{
			if(temperature_buffer.type == BMS_DATA_TEMPERATURE)
			{


				if((temperature_buffer.slave_id % CAN_DATA_PER_FRAME == 1) &&  (temperature_buffer.slave_id != prev_first_ID))
				{
					if(prev_first_ID != 0)
					{
						can_buffer.first_slave_id = temperature_buffer.slave_id - CAN_DATA_PER_FRAME + 1;
						xQueueSendToBack(bmsCanQueue, &can_buffer, (TickType_t)10);
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
					xQueueSendToBack(bmsCanQueue, &can_buffer, (TickType_t)10);
					memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
					can_buffer.Data.frame.slave[(temperature_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = temperature_buffer.value;
					prev_first_ID = temperature_buffer.slave_id;
				}

//					can_buffer.Data.frame.slave[(temperature_buffer.slave_id - 1) % CAN_DATA_PER_FRAME] = temperature_buffer.value;
//
//					if((temperature_buffer.slave_id % CAN_DATA_PER_FRAME) == 0)
//					{
//						can_buffer.first_slave_id = temperature_buffer.slave_id - CAN_DATA_PER_FRAME + 1;
//						xQueueSendToBack(bmsCanQueue, &can_buffer, (TickType_t)10);
//						memset(&can_buffer.Data, 0, sizeof(can_buffer.Data));
//					}
//

			}
			else
			{
				xQueueSendToBack(bmsMeasurmentsQueue, &temperature_buffer, (TickType_t)10);
				continue;
			}
		}
		else
		{
			//indicate message is not received
			//todo Optimize wait delay to be dynamic to the number of slaves
			xTaskNotifyWait(0, NOTIFY_BMS_GOT_TEMP, NULL, pdMS_TO_TICKS(portMAX_DELAY));
		}

	}
}


void BMS_CanTask(void const * argument)
{
	BMS_CAN_Queue_Message_t CAN_Queue_Buffer;
	BMS_CAN_TxHandler.ExtId = 0x00;                 //Message ID extension for CAN extend
	BMS_CAN_TxHandler.IDE = CAN_ID_STD;             //CAN ID is 11 bits
	BMS_CAN_TxHandler.RTR = CAN_RTR_DATA;           //Set RTR bit to 0(sends data)
	BMS_CAN_TxHandler.DLC = CAN_MSG_SIZE;           //Data sent is CAN_MSG_SIZE bytes in BMS_Config.h


	xEventGroupWaitBits(BMS_EventGroup, BMS_INIT_DONE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

	for(;;)
	{
		if(xQueueReceive(bmsCanQueue, &CAN_Queue_Buffer, (TickType_t)10) == pdTRUE)
		{
			//todo SET proper IDS
			if(CAN_Queue_Buffer.type == BMS_DATA_VOLTAGE)
			{
				BMS_CAN_TxHandler.StdId = CAN_VOLT_ID + ((CAN_Queue_Buffer.first_slave_id - 1) / CAN_DATA_PER_FRAME);                //Message ID
			}
			else if(CAN_Queue_Buffer.type == BMS_DATA_TEMPERATURE)
			{
				BMS_CAN_TxHandler.StdId = CAN_TEMP_ID + ((CAN_Queue_Buffer.first_slave_id - 1) / CAN_DATA_PER_FRAME);                //Message ID
			}

			if( HAL_CAN_AddTxMessage(&hcan, &BMS_CAN_TxHandler, (uint8_t*)&(CAN_Queue_Buffer.Data), &TxMailbox) != HAL_OK)
			{
				//error
				HAL_UART_Transmit(&huart2, (uint8_t*)"CAN Failed", 10, HAL_MAX_DELAY);
			}

//			vTaskDelay(pdMS_TO_TICKS(50));
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
