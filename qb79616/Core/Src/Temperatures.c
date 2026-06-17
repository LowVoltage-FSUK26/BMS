/*
 * Temperature.c
 *
 *  Created on: Apr 12, 2026
 *      Author: AbdUllah
 */


#include "stm32f1xx_hal.h"
#include "bq79616.h"
#include "bq79600.h"
#include "BMS_Config.h"
#include "Temperatures.h"

uint16_t GpioReadings[SLAVEBOARDS][4];
// Function to configure GPIO1 as specified input
uint8_t configureGPIO(uint8_t GPIO_NUM, BQ79616_GPIO_Config_t GPIO_MODE ,uint8_t BID, uint8_t bWriteType){

	if(BID >= SLAVEBOARDS)
		return 2;		// Invalid Board ID

	if (GPIO_NUM < 1 || GPIO_NUM > 8)
	    return 3;   	// Invalid GPIO

	if (GPIO_MODE > 0x07)
	    return 4;		// Value must fit in 3 bits

	if ((bWriteType != FRMWRT_SGL_W) && (bWriteType != FRMWRT_STK_W) )
	    return 5;		// Invalid bWriteType

	uint8_t reg_addr = BQ79616_GPIO_CONF1 + ((GPIO_NUM - 1)/2);
	uint8_t reg_val = (GPIO_MODE << (!(GPIO_NUM % 2) * 3));
	writeReg((bWriteType == FRMWRT_SGL_W)?BID : 0, reg_addr, reg_val, 1, bWriteType);
	return 1; 			//Correct Config
}

uint8_t configure_OTUT(uint8_t dev_address, uint8_t activeThermistors){

	uint8_t ot_ut = 0xE0;  //reset value UT= 80% and OT= 39%
	uint8_t cb_coolOff= 0x0F; //reset values of OTCB_THR and COOLOFF hysteresis
	uint8_t dev_stat;
	uint8_t goCmd = 0x05;
	uint8_t gpioConf= 0x09; //for simplicity enable all gpio thermistors

	//set UT and OT thresholds
	writeReg(dev_address, BQ79616_OTUT_THRESH, ot_ut ,1 , FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_OTCB_THRESH, cb_coolOff ,1 , FRMWRT_SGL_W);

	//enable TSERF
	writeReg(dev_address, BQ79616_CONTROL2, 0x01, 1, FRMWRT_SGL_W);
	vTaskDelay(2);

	//configure all GPIOs for thermistors
	/*
	writeReg(dev_address, BQ79616_GPIO_CONF1, gpioConf, 1, FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_GPIO_CONF2, gpioConf, 1, FRMWRT_SGL_W);
	writeReg(dev_address, BQ79616_GPIO_CONF3, gpioConf, 1, FRMWRT_SGL_W);
	 */

	writeReg(dev_address, BQ79616_GPIO_CONF1, gpioConf, 1, FRMWRT_SGL_W);// Enable GPIO1,2 for thermistor
	writeReg(dev_address, BQ79616_GPIO_CONF2, gpioConf, 1, FRMWRT_SGL_W);// Enable GPIO3,4 for thermistor
	//set OTUT mode
	writeReg(dev_address, BQ79616_OTUT_CTRL, OTUT_MODE,1 , FRMWRT_SGL_W);

	//set Vcb_done to Vuv

	//Start Protection
	writeReg(dev_address, BQ79616_OTUT_CTRL, goCmd ,1 , FRMWRT_SGL_W);

	//read back protection status to ensure its ON
	readReg(dev_address, DEV_STAT, &dev_stat, 1, 200, FRMWRT_SGL_R);
	if((dev_stat& 0x10) == 0){
		return 0;   //error OTUT is not enabled
	}
	return 1;
}


uint16_t readGPIOVoltage(uint8_t BID, uint8_t GPIO_NUM, uint16_t* raw_value_ptr) {


	int16_t raw_value = 0;
	uint8_t stat = 0;
	uint32_t timeout = 100;
	while(timeout--) {
	    readReg(BID, ADC_STAT1, &stat, 1, 0, FRMWRT_SGL_R);
	    if(stat & 0x01) break;  // Bit3 = DRDY_MAIN_ADC
	    vTaskDelay(1);
	}
	if((GPIO_NUM >= 1) && (GPIO_NUM <= 8))
	{

		uint8_t raw[2];
		readReg(BID, GPIO1_HI + 2*(GPIO_NUM-1), raw, 2, 0, FRMWRT_SGL_R);
		raw_value = (int16_t)((raw[0] << 8) | raw[1]);

		*raw_value_ptr = raw_value;
	}
	return raw_value;
}


