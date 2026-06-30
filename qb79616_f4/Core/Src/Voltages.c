/*
 * Voltages.c
 *
 *  Created on: Mar 15, 2026
 *      Author: AbdUllah
 */

#include "stm32f4xx_hal.h"
#include "bq79616.h"
#include "bq79600.h"
#include "BMS_Config.h"
#include "Voltages.h"
uint16_t cellVoltages_board[SLAVEBOARDS][16] = {0};
float cellVoltages_board_float[SLAVEBOARDS][16];

void initADC(void) {
    writeReg(0, BQ79616_ADC_CTRL1, 0x06, 1, FRMWRT_STK_W);
    vTaskDelay(10);

    uint8_t dev_stat;
    for (uint8_t i = 1; i <= SLAVEBOARDS; i++) {
        readReg(i, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
        if ((dev_stat & 0x01) == 0) {
            while(1);
        }
    }
}
uint8_t configure_OVUV(uint8_t dev_address , uint8_t activeCells){

	uint8_t dev_stat;
	uint8_t goCmd = 0x05;

	//set active cells number
	writeReg(dev_address, BQ79616_ACTIVE_CELL, activeCells - 6, 1, FRMWRT_SGL_W);
	//set OV and UV thresholds
	writeReg(dev_address, BQ79616_OV_THRESH, OV_THR, 1, FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_UV_THRESH, UV_THR, 1, FRMWRT_SGL_W);

	//set OVUV mode
	writeReg(dev_address, BQ79616_OVUV_CTRL, OVUV_MODE,1 , FRMWRT_SGL_W);

	//Start Protection
	writeReg(dev_address, BQ79616_OVUV_CTRL, goCmd ,1 , FRMWRT_SGL_W);

	//read back protection status to ensure its ON
	readReg(dev_address, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
	//HAL_Delay(50);
	if((dev_stat& 0x08) == 0){
		return 0;   //error OVUV is not enabled
	}
	return 1;
}

void configureAll_OVUV(void) {
    for (uint8_t i = 1; i <= SLAVEBOARDS; i++) {
        if (configure_OVUV(i, slaveCellCount[i-1]) != 1) {
            while(1); // error handler
        }
    }
}

uint8_t readCellVoltages(uint8_t boardNum, uint8_t numCells, int *totalV){
	int cellVoltages[16] = {0}; // Index 0 = Cell16, index 15 = Cell1

	uint8_t hiBytedata = 0, loByteData = 0;

	int cell_voltage = 0;
	*totalV = 0;
	//for reading 2 regs at once
	uint8_t full_cell_voltage[2];

	for (uint8_t cell = 1; cell <= numCells; cell++) {
		uint16_t hiRegAddr = BQ79616_CELL_VOLTAGE_BASE + ((16 - cell) * 2);
		uint16_t loRegAddr = hiRegAddr + 1;

		// Read the high byte
		readReg(boardNum, hiRegAddr, &hiBytedata, 1, 100, FRMWRT_SGL_R);
		// Read the low byte
		//readReg(boardNum, loRegAddr, loByteFrame, 1, 100, FRMWRT_SGL_R);


		/*
				// Read the high byte
        if (readReg(boardNum, hiRegAddr, &hiBytedata, 1, 100, FRMWRT_SGL_R) < 1) {
            continue;
        }

        // Read the low byte
        if (readReg(boardNum, loRegAddr, &loByteData, 1, 100, FRMWRT_SGL_R) < 1) {
            continue;
        }
		 */


		// Combine high and low bytes into a signed 16-bit ADC value (2's complement)
		cell_voltage = (int)((hiBytedata << 8) | loByteData);


		//alt read 2 regs at once:
		//readReg(boardNum, hiRegAddr, full_cell_voltage, 2, 100, FRMWRT_SGL_R);
		//cell_voltage = (int)((full_cell_voltage[0] << 8) | full_cell_voltage[1]);
		// Store voltage in correct index
		cellVoltages[cell - 1] = cell_voltage;
		*totalV += cell_voltage;
	}
	if(*totalV == 0){
		return 0;
	}
	return 1;
}

uint8_t readBoardVoltages(uint8_t boardNum, uint8_t numCells, int *totalV, uint16_t *cellVoltages) {
	int16_t cell_voltage = 0;
	*totalV = 0;

	uint8_t full_cell_voltage[2];

	uint8_t stat = 0;
	uint32_t timeout = 100;
	while(timeout--) {
	    readReg(boardNum, ADC_STAT1, &stat, 1, 0, FRMWRT_SGL_R);
	    if(stat & 0x01) break;  // DRDY_MAIN_ADC
	    vTaskDelay(1);
	}

	for (uint8_t cell = 1; cell <= numCells; cell++) {
		uint16_t hiRegAddr = BQ79616_CELL_VOLTAGE_BASE + ((16 - cell) * 2);


		// Read two registers at once
		if (readReg(boardNum, hiRegAddr, full_cell_voltage, 2, 200, FRMWRT_SGL_R) < 1) {
			continue;  // Skip this cell if read fails
		}

		cell_voltage = (int16_t)((full_cell_voltage[0] << 8) | full_cell_voltage[1]);

		// Store voltage in correct index
		cellVoltages[cell - 1] = cell_voltage;
		*totalV += cell_voltage;
	}

	return (*totalV == 0) ? 0 : 1;
}
uint8_t readAllBoardVoltages(void) {

    uint8_t status = 1;
    int totalV;

    for (uint8_t s = 1; s <= SLAVEBOARDS; s++) {
        if (!readBoardVoltages(s, slaveCellCount[s-1], &totalV, cellVoltages_board[s-1])) {
            status = 0;
        }
    }

    return status;
}
void convertVoltages(void) {
    for (uint8_t s = 0; s < SLAVEBOARDS; s++)
        for (uint8_t c = 0; c < 16; c++)
            cellVoltages_board_float[s][c] = cellVoltages_board[s][c] * 0.00019073f;
}
float getTotalVoltage(void) {
    float total = 0.0f;

    //  (slaves: 0,1 / 8,9 / 16,17 / 24,25)
    for (uint8_t seg = 0; seg < SLAVEBOARDS/8; seg++) {
        uint8_t s1 = seg * 8;       // slave 14 cells
        uint8_t s2 = seg * 8 + 1;   // slave 13 cells

        for (uint8_t c = 0; c < slaveCellCount[s1]; c++)
            total += cellVoltages_board_float[s1][c];

        for (uint8_t c = 0; c < slaveCellCount[s2]; c++)
            total += cellVoltages_board_float[s2][c];
    }
    return total;
}
