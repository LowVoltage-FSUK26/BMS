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
#define TOTALBOARDS 3       //boards in stack (including the bridge)
#define SLAVEBOARDS	(TOTALBOARDS - 1)
#define Thermistor_GPIO 8
#define BRIDGEUART	1
#define DEBUGUART	2


//---------------
//	Topology
//---------------

//#define RING
#define CHAIN


/* ------------------------------------
     BMS Configuration MACROS
   ------------------------------------
*/
#define CAN_MSG_SIZE           8               //Size in bytes

typedef uint8_t CAN_Data_Type_t;

#define VOLT  ((CAN_Data_Type_t)0)
#define TEMP  ((CAN_Data_Type_t)1)

/* ------------------------------------
     BMS Frame Structs
   ------------------------------------
*/


typedef struct {

	uint16_t Slave_1;	//Total Voltage/TEMP of 1st Slave in Frame
	uint16_t Slave_2;	//Total Voltage/TEMP of 2nd Slave in Frame
	uint16_t Slave_3;	//Total Voltage/TEMP of 3rd Slave in Frame
	uint16_t Index;		//Index of the 1st and 3rd slave in frame in relation to total chain
						//In case of last frame in chain, It holds TOTAL Battery Voltage/TEMP

} BMS_CAN_Frame_t;

typedef struct {

	BMS_CAN_Frame_t Data;
	CAN_Data_Type_t type;			//Voltage or Temp


} BMS_CAN_Queue_element_t;


//---------------------
//RTOS Design MODE
//---------------------
//#define SIMPLETASK

#define EVENT_GROUP

/* ------------------------------------
     BMS Configuration Typedefs
   ------------------------------------
*/


typedef enum {
    CMD_READ_CELL_VOLTAGES,
    CMD_READ_GPIO_ADC,
	CMD_READ_BRIDGE_FS
} BMS_Command_t;

typedef struct {
    BMS_Command_t cmd;
    TaskHandle_t  requester;   // who asked (for notification)
    uint8_t GPIO_NUM;
    uint8_t BOARD_NUM;
} BMS_Request_t;
void Bridge_FaultInit(void);
void Bridge_CheckFaults(void);
void Stack_FaultInit(void);
void Stack_CheckFaultSummary(void);

/* ------------------------------------
     BMS Configuration Prototypes
   ------------------------------------
*/
void BMS_Can_Init(void);




#endif /* INC_BMS_CONFIG_H_ */
