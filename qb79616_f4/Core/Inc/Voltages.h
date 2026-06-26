/*
 * Voltages.h
 *
 *  Created on: Apr 12, 2026
 *      Author: AbdUllah
 */

#ifndef INC_VOLTAGES_H_
#define INC_VOLTAGES_H_
extern uint16_t cellVoltages_board[SLAVEBOARDS][16];
extern uint8_t slaveCellCount[SLAVEBOARDS] ;
uint8_t configure_OVUV(uint8_t dev_address , uint8_t activeCells);
uint8_t readCellVoltages(uint8_t boardNum, uint8_t numCells, int *totalV);
uint8_t readBoardVoltages(uint8_t boardNum, uint8_t numCells, int *totalV, uint16_t *cellVoltages);
uint8_t readAllBoardVoltages(void);
#endif /* INC_VOLTAGES_H_ */
