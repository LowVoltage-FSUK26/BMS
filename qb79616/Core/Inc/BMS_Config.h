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

//---------------------
//RTOS Design MODE
//---------------------
//#define SIMPLETASK
//#define ADVANCETASK
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


#endif /* INC_BMS_CONFIG_H_ */
