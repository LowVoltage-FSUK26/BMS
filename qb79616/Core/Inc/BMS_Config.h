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
#define TOTALBOARDS 3       //boards in stack
#define SLAVEBOARDS	(TOTALBOARDS - 1)


/* ------------------------------------
     BMS Configuration Typedefs
   ------------------------------------
*/
typedef enum {
    CMD_READ_CELL_VOLTAGES,
    CMD_READ_GPIO_ADC
} BMS_Command_t;

typedef struct {
    BMS_Command_t cmd;
    TaskHandle_t  requester;   // who asked (for notification)
} BMS_Request_t;


#endif /* INC_BMS_CONFIG_H_ */
