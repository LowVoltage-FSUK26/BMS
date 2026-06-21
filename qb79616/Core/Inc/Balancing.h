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
#define BAL_CTRL2_RUN        0x33    // AUTO_BAL | BAL_GO | OTCB_EN | FLTSTOP_EN
#define BAL_CTRL2_STOP       0x00
#define BAL_LOOP_DELAY_MS    60000   // 60 seconds

#endif /* INC_BALANCING_H_ */
