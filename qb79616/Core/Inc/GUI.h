/*
 * GUI.h
 *
 *  Created on: Mar 14, 2026
 *      Author: AbdUllah
 */

#ifndef INC_GUI_H_
#define INC_GUI_H_

void Send_GUI_Reading (void);
void UART_Send_Board_Frame_CharByChar(uint8_t board_num);
void uint8_to_hex2( char *out,uint8_t value);
void uint16_to_hex4( char *out,uint16_t value);


#endif /* INC_GUI_H_ */
