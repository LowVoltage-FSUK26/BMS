/*
 * Balancing.c
 *
 *  Created on: Jun 20, 2026
 *      Author: AbdUllah
 */
#include "stm32f1xx_hal.h"
#include "bq79616.h"
#include "bq79600.h"
#include "BMS_Config.h"
#include "Voltages.h"
#include "Balancing.h"

/* ─────────────────────────────────────────────
   Helper: Find global Vmin across all slaves
───────────────────────────────────────────── */
uint16_t findGlobalVmin(uint16_t voltages[SLAVEBOARDS][16]) {
	uint16_t vmin = voltages[0][0];

	for (uint8_t s = 0; s < SLAVEBOARDS; s++) {
		for (uint8_t c = 0; c < NUM_CELLS; c++) {
			if (voltages[s][c] < vmin)
				vmin = voltages[s][c];
		}
	}
	return vmin;
}

/* ─────────────────────────────────────────────
   Helper: Find global Vmax across all slaves
───────────────────────────────────────────── */
uint16_t findGlobalVmax(uint16_t voltages[SLAVEBOARDS][16]) {
	uint16_t vmax = voltages[0][0];

	for (uint8_t s = 0; s < SLAVEBOARDS; s++) {
		for (uint8_t c = 0; c < NUM_CELLS; c++) {
			if (voltages[s][c] > vmax)
				vmax = voltages[s][c];
		}
	}
	return vmax;
}



/* ─────────────────────────────────────────────
   Initialization — call once at charging start
───────────────────────────────────────────── */
void balancing_init(void) {

	for (uint8_t s = 1; s <= SLAVEBOARDS; s++) {

		// 1. Set balancing timer for all cells (30 min)
		for (uint8_t c = 0; c < NUM_CELLS; c++) {
			writeReg(s, BQ79616_CB_CELL_CTRL_01  - c, 0x09, 1, FRMWRT_SGL_W);
		}

		// 2. Enable OTCB thermal protection
		//    GPIO must already be configured as ADC+OTUT (done in configure_OTUT)
		// writeReg(s, BQ79616_OTCB_THRESH, 0x0F, 1, FRMWRT_SGL_W);

		// 3. VCB_DONE_THRESH = 0 for now (will be set in update loop)
		writeReg(s, BQ79616_VCB_DONE_THRESH, 0x00, 1, FRMWRT_SGL_W);

		// 4. Start UV protector — required for VCB_DONE detection
		writeReg(s, BQ79616_OVUV_CTRL, 0x05, 1, FRMWRT_SGL_W);

		// 5. Start balancing
		writeReg(s, BQ79616_BAL_CTRL2, BAL_CTRL2_STOP, 1, FRMWRT_SGL_W);
	}
}
static uint8_t last_CB_THR[SLAVEBOARDS] = {0};
void balancing_update(void) {


	// 4. Find global Vmin and Vmax
	uint16_t vmin = findGlobalVmin(cellVoltages_board);
	uint16_t vmax = findGlobalVmax(cellVoltages_board);

	// 5. Check if balancing is needed
	if ((vmax - vmin) < BALANCE_DELTA_LSB) {
		// Pack is balanced — stop balancing

		for (uint8_t c = 0; c < NUM_CELLS; c++) {
			writeReg(0, BQ79616_CB_CELL_CTRL_01- c, 0x00, 1, FRMWRT_STK_W);
		}

		for (uint8_t s = 1; s <= SLAVEBOARDS; s++) {
			writeReg(s, BQ79616_BAL_CTRL2, BAL_CTRL2_STOP, 1, FRMWRT_SGL_W);
		}
		// or writeReg(0, BQ79616_BAL_CTRL2, BAL_CTRL2_STOP, 1, FRMWRT_STK_W);
		return;
	}

	// 6. Calculate new VCB_DONE threshold
	//    target = 10mV below vmin
	float vmin_volts = vmin * 0.00019073f;


	// Clamp to valid register range (2.45V to 4.0V)
	if (vmin_volts < 2.45f) vmin_volts = 2.45f;
	if (vmin_volts > 4.00f) vmin_volts = 4.00f;

	uint8_t new_CB_THR = (uint8_t)((vmin_volts - 2.45f) / 0.025f)+1;

	// 7. Update each slave only if threshold changed
	for (uint8_t s = 1; s <= SLAVEBOARDS; s++) {

		if (new_CB_THR != last_CB_THR[s-1]){//continue // no change, skip
			// Write new threshold
			writeReg(s, BQ79616_VCB_DONE_THRESH, new_CB_THR, 1, FRMWRT_SGL_W);

			// Restart UV protector to latch new threshold
			writeReg(s, BQ79616_OVUV_CTRL, 0x05, 1, FRMWRT_SGL_W);
		}

		// Rewrite timers with remaining values
		for (uint8_t c = 0; c < NUM_CELLS; c++) {
			uint8_t timer = ((cellVoltages_board[s-1][c] - vmin) > BALANCE_DELTA_LSB)? 0x09: 0x00;
			writeReg(s, BQ79616_CB_CELL_CTRL_01- c, timer, 1, FRMWRT_SGL_W);
		}



		// Re-fire BAL_GO
		writeReg(s, BQ79616_BAL_CTRL2, BAL_CTRL2_RUN, 1, FRMWRT_SGL_W);

		last_CB_THR[s-1] = new_CB_THR;
	}
}
