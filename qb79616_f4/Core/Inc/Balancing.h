/*
 * Balancing.h
 *
 *  Created on: Jun 20, 2026
 *      Author: AbdUllah
 */

#ifndef INC_BALANCING_H_
#define INC_BALANCING_H_

#define NUM_CELLS            14
#define BALANCE_DELTA_LSB    105     // ~20mV  (20 / 0.19073)
#define BAL_CTRL2_RUN        0x23    // AUTO_BAL | BAL_GO  | FLTSTOP_EN
#define BAL_CTRL2_STOP       0x00
#define BAL_LOOP_DELAY_MS    60000   // 60 seconds

void balancing_init(void);
void balancing_update(void);

#endif /* INC_BALANCING_H_ */
