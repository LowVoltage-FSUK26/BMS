/*
 * Balancing.h
 *
 *  Created on: Jun 20, 2026
 *      Author: AbdUllah
 */

#ifndef INC_BALANCING_H_
#define INC_BALANCING_H_


#define BALANCE_DELTA_LSB    105     // ~20mV  (20 / 0.19073)
#define BAL_CTRL2_RUN        0x03    // AUTO_BAL | BAL_GO  | FLTSTOP_EN
#define BAL_LOOP_DELAY_MS    60000   // 60 seconds

typedef struct {
    uint8_t cb_done      : 1;  // bit 0
    uint8_t mb_done      : 1;  // bit 1
    uint8_t abortflt     : 1;  // bit 2
    uint8_t cb_run       : 1;  // bit 3
    uint8_t mb_run       : 1;  // bit 4
    uint8_t cb_inpause   : 1;  // bit 5
    uint8_t ot_pause_det : 1;  // bit 6
    uint8_t invalid_cbconf : 1; // bit 7
} BAL_STAT_t;


extern BAL_STAT_t BalStatReadings[SLAVEBOARDS];
extern uint16_t CB_CompleteReadings[SLAVEBOARDS];
void balancing_init(void);
void balancing_update(void);
void balancing_stop(void);
void readBAL_STAT(void);
BAL_STAT_t parse_BAL_STAT(uint8_t raw);
#endif /* INC_BALANCING_H_ */
