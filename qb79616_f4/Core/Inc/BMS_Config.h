/*
 * BMS_Config.h
 *
 *  Created on: Feb 7, 2026
 *      Author: Mina Fathy Labib
 */

#ifndef INC_BMS_CONFIG_H_
#define INC_BMS_CONFIG_H_


/* ------------------------------------
     BMS Utilized Libraries
   ------------------------------------
 */
#include "bq79600.h"	// BQ79600 definitions and macros
#include "bq79616.h"	// BQ79616 definitions and macros
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"


/* ------------------------------------
     BMS Configuration MACROS
   ------------------------------------
 */
#define TOTALBOARDS 		1      //boards in stack (including the bridge)
#define SLAVEBOARDS			(TOTALBOARDS - 1)
#define ACTIVE_CELLS		16
#define Thermistor_GPIO 	2
#define BRIDGEUART			1
#define DEBUGUART			2
#define BATTERYVOLT_ID		(TOTALBOARDS + (4 - (TOTALBOARDS % 4)))

#define VOLT_CONV		0.00019073
#define GUI

#define MAX(a, b)  ((a) > (b) ? (a) : (b))



//---------------
//	Topology
//---------------

//#define RING
#define CHAIN


/* ------------------------------------
     BMS Configuration MACROS
   ------------------------------------
 */
#define CAN_MSG_SIZE           	8               //Size in bytes
#define CAN_DATA_SIZE			2
#define CAN_DATA_PER_FRAME		(CAN_MSG_SIZE/CAN_DATA_SIZE)
#define CAN_VOLT_ID				0x100
#define CAN_TEMP_ID				0x200
#define CAN_BMS_TO_CH			0x1806E5F4
#define CAN_CH_TO_BMS			0x18FF50E5
#define CAN_BMS_TO_CH_STD		((CAN_BMS_TO_CH & 0x1FFC0000) >> 18)
#define CAN_BMS_TO_CH_ExID		(CAN_BMS_TO_CH & 0x0003FFFF)


/* ------------------------------------
     CAN Charger MACROS
   ------------------------------------
*/
#define CHAR_MAX_VOLT			1700		//0.1V/byte  offset:0  e.g. Vset=3201, its corresponding 320.1V
#define CHAR_MAX_VOLT_High		((CHAR_MAX_VOLT & 0xFF00) >> 8)
#define CHAR_MAX_VOLT_low		(CHAR_MAX_VOLT & 0x00FF)


#define CHAR_MAX_AMP			10		//0.1A/byte  offset:0  e.g. Iset=582, its corresponding 58.2A
#define CHAR_MAX_AMP_High		((CHAR_MAX_AMP & 0xFF00) >> 8)
#define CHAR_MAX_AMP_low		(CHAR_MAX_AMP & 0x00FF)

//Control
#define CHAR_START				0
#define CHAR_STOP				1

/* ------------------------------------
     Current Sensor MACROS
   ------------------------------------
*/
#define HASS_50_S_Vof 	2.5
#define HASS_50_S_IPN  	50
#define NUM_SAMPLES 	10
#define NUM_CHANNELS 	2

/* ------------------------------------
     BMS Frame Structs
   ------------------------------------
 */
typedef enum
{
  BMS_OK       = 0x00U,
  BMS_ERROR    = 0x01U,
  BMS_BUSY     = 0x02U,
  BMS_TIMEOUT  = 0x03U
} BMS_StatusTypeDef;

typedef union
{
	uint8_t bytes[8];

	struct {

		uint16_t slave[4];	//Total Voltage/TEMP of each slave
							//In case of last frame in chain, It holds TOTAL Battery Voltage/TEMP
	} frame;

} BMS_CAN_Frame_t;

typedef union //to be used in future
{
	uint8_t bytes[8];

	struct {

		uint16_t max_char_VOTL;
		uint16_t max_char_AMP;
		uint8_t  control;

	} frame;

} CHARGER_CAN_Frame_t;

//---------------------
//RTOS Design MODE
//---------------------

//#define SIMPLETASK
#define EVENT_GROUP

//---------------------
//RTOS MACROS
//--------------------
#define NOTIFY_BMS_INIT_DONE   		(1U << 0)
#define NOTIFY_BMS_GOT_MSG          (1U << 1)       //General Message
#define NOTIFY_BMS_GOT_VOLT         (1U << 2)
#define NOTIFY_BMS_GOT_TEMP         (1U << 3)
#define NOTIFY_BMS_GOT_FS           (1U << 4)



/* ------------------------------------
     BMS Configuration Typedefs
   ------------------------------------
 */

typedef enum
{
    BMS_DATA_VOLTAGE = 0,
    BMS_DATA_TEMPERATURE = 1,
	BMS_CHARGER = 2

} BMS_DataType_t;

typedef struct
{
	int16_t value;
    uint8_t slave_id;
    uint8_t  _pad;

} BMS_Queue_Measurement_t;


typedef struct {

	BMS_CAN_Frame_t Data;
	BMS_DataType_t type;			//Voltage or Temp
	uint8_t first_slave_id;

} BMS_CAN_Queue_Message_t;




//Used for commands sent to BMS_CommadTask to indicate the type of Command
typedef enum {
	CMD_READ_CELL_VOLTAGES,
	CMD_READ_GPIO_ADC,
	CMD_READ_BRIDGE_FS
} BMS_Command_t;

//Used for Queue Command Messages sent to BMS_CommadTask
typedef struct {
	BMS_Command_t cmd;
	TaskHandle_t  requester;   // who asked (for notification)
	uint8_t GPIO_NUM;
	uint8_t BOARD_NUM;
} BMS_Request_t;


/* ------------------------------------
     BMS Configuration Prototypes
   ------------------------------------
 */
void BMS_Can_Init(void);




#endif /* INC_BMS_CONFIG_H_ */
