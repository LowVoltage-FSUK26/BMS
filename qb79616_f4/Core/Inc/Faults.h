/*
 * Faults.h
 *
 *  Created on: Apr 12, 2026
 *      Author: AbdUllah
 */

#ifndef INC_FAULTS_H_
#define INC_FAULTS_H_
#include "event_groups.h"

extern uint8_t bridge_faultSummary ;
extern uint8_t slaveFaultS[SLAVEBOARDS];

extern EventGroupHandle_t faultEventGroup;
#define FAULT_EVENT_BIT   (1 << 0)

void readAllFaultSummaries(void);
void Bridge_FaultInit(void);
void Bridge_CheckFaults(void);
void Stack_FaultInit(void);
void Stack_CheckFaultSummary(void);
#endif /* INC_FAULTS_H_ */
