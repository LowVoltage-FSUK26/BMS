/*
 * Temperature.c
 *
 *  Created on: Apr 12, 2026
 *      Author: AbdUllah
 */


#include "stm32f4xx_hal.h"
#include "bq79616.h"
#include "bq79600.h"
#include "BMS_Config.h"
#include "Temperatures.h"

#include <math.h>
#include <stdint.h>
uint16_t GpioReadings[SLAVEBOARDS][NUM_GPIOS] = {0};
uint16_t TsrefReadings[SLAVEBOARDS] = {0};
float temperatures[SLAVEBOARDS][NUM_GPIOS] = {0.0f};
// Function to configure GPIO1 as specified input
uint8_t configureGPIO(uint8_t GPIO_NUM, BQ79616_GPIO_Config_t GPIOMODE ,uint8_t BID, uint8_t bWriteType){

	if(BID >= SLAVEBOARDS)
		return 2;		// Invalid Board ID

	if (GPIO_NUM < 1 || GPIO_NUM > 8)
	    return 3;   	// Invalid GPIO

	if (GPIOMODE > 0x07)
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

void readAllGPIO(void) {

    for (uint8_t s = 1; s <= SLAVEBOARDS; s++) {
        // Read GPIO1–4
        for (uint8_t g = 0; g < NUM_GPIOS; g++) {
            readGPIOVoltage(s, g + 1, &GpioReadings[s-1][g]);
        }
    }
}

void readTSREF(void) {

    for (uint8_t s = 1; s <= SLAVEBOARDS; s++) {

        uint8_t stat = 0;
        uint32_t timeout = 100;
        while (timeout--) {
            readReg(s, ADC_STAT1, &stat, 1, 0, FRMWRT_SGL_R);
            if (stat & 0x01) break;  // DRDY_MAIN_ADC
            vTaskDelay(1);
        }

        uint8_t raw[2];
        readReg(s, TSREF_HI, raw, 2, 0, FRMWRT_SGL_R);

        TsrefReadings[s-1] = (uint16_t)((raw[0] << 8) | raw[1]);
    }
}




static const NTC_Point ntc_table[] = {
    { -40, 335746 },
    { -39, 314203 },
    { -38, 294177 },
    { -37, 275554 },
    { -36, 258227 },
    { -35, 242098 },
    { -34, 227077 },
    { -33, 213082 },
    { -32, 200037 },
    { -31, 187871 },
    { -30, 176521 },
    { -29, 165926 },
    { -28, 156034 },
    { -27, 146792 },
    { -26, 138154 },
    { -25, 130077 },
    { -24, 122523 },
    { -23, 115453 },
    { -22, 108834 },
    { -21, 102636 },
    { -20,  96828 },
    { -19,  91383 },
    { -18,  86278 },
    { -17,  81489 },
    { -16,  76995 },
    { -15,  72776 },
    { -14,  68813 },
    { -13,  65090 },
    { -12,  61590 },
    { -11,  58300 },
    { -10,  55205 },
    {  -9,  52292 },
    {  -8,  49551 },
    {  -7,  46969 },
    {  -6,  44538 },
    {  -5,  42246 },
    {  -4,  40086 },
    {  -3,  38049 },
    {  -2,  36127 },
    {  -1,  34314 },
    {   0,  32602 },
    {   1,  30986 },
    {   2,  29459 },
    {   3,  28016 },
    {   4,  26653 },
    {   5,  25363 },
    {   6,  24143 },
    {   7,  22989 },
    {   8,  21897 },
    {   9,  20863 },
    {  10,  19884 },
    {  11,  18956 },
    {  12,  18076 },
    {  13,  17243 },
    {  14,  16453 },
    {  15,  15703 },
    {  16,  14992 },
    {  17,  14317 },
    {  18,  13676 },
    {  19,  13067 },
    {  20,  12489 },
    {  21,  11940 },
    {  22,  11418 },
    {  23,  10921 },
    {  24,  10449 },
    {  25,  10000 },
    {  26,   9573 },
    {  27,   9166 },
    {  28,   8779 },
    {  29,   8410 },
    {  30,   8059 },
    {  31,   7724 },
    {  32,   7405 },
    {  33,   7101 },
    {  34,   6812 },
    {  35,   6535 },
    {  36,   6271 },
    {  37,   6020 },
    {  38,   5779 },
    {  39,   5550 },
    {  40,   5331 },
    {  41,   5122 },
    {  42,   4922 },
    {  43,   4731 },
    {  44,   4548 },
    {  45,   4373 },
    {  46,   4206 },
    {  47,   4047 },
    {  48,   3894 },
    {  49,   3748 },
    {  50,   3608 },
    {  51,   3474 },
    {  52,   3345 },
    {  53,   3222 },
    {  54,   3105 },
    {  55,   2992 },
    {  56,   2883 },
    {  57,   2780 },
    {  58,   2680 },
    {  59,   2585 },
    {  60,   2493 },
    {  61,   2406 },
    {  62,   2321 },
    {  63,   2240 },
    {  64,   2163 },
    {  65,   2088 },
    {  66,   2017 },
    {  67,   1948 },
    {  68,   1882 },
    {  69,   1818 },
    {  70,   1757 },
    {  71,   1698 },
    {  72,   1642 },
    {  73,   1588 },
    {  74,   1535 },
    {  75,   1485 },
    {  76,   1437 },
    {  77,   1390 },
    {  78,   1345 },
    {  79,   1302 },
    {  80,   1261 },
    {  81,   1221 },
    {  82,   1182 },
    {  83,   1145 },
    {  84,   1109 },
    {  85,   1075 },
    {  86,   1041 },
    {  87,   1009 },
    {  88,    978 },
    {  89,    948 },
    {  90,    920 },
    {  91,    892 },
    {  92,    865 },
    {  93,    839 },
    {  94,    814 },
    {  95,    790 },
    {  96,    767 },
    {  97,    744 },
    {  98,    723 },
    {  99,    702 },
    { 100,    681 },
    { 101,    662 },
    { 102,    643 },
    { 103,    624 },
    { 104,    607 },
    { 105,    590 },
    { 106,    573 },
    { 107,    557 },
    { 108,    542 },
    { 109,    527 },
    { 110,    512 },
    { 111,    498 },
    { 112,    484 },
    { 113,    471 },
    { 114,    459 },
    { 115,    446 },
    { 116,    434 },
    { 117,    423 },
    { 118,    411 },
    { 119,    401 },
    { 120,    390 },
    { 121,    380 },
    { 122,    370 },
    { 123,    360 },
    { 124,    351 },
    { 125,    342 },
};


float convertNTCtoTemp(uint16_t raw_adc,uint16_t raw_TSREF) {

    // Step 1: raw → voltage
    float V_gpio = raw_adc * 0.00015259f;
    float V_TSREF = raw_TSREF * 0.00015259f;
    // Guard against invalid readings
    if (V_gpio <= 0.0f || V_gpio >= V_TSREF)
        return -273.15f;  // error sentinel

    // Step 2: voltage → resistance
    float R_ntc = R_PULLUP * V_gpio /(V_TSREF - V_gpio) ;

    // Step 3: resistance → temperature via lookup + interpolation

    // Below table range
    if (R_ntc >= ntc_table[0].resistance)
        return (float)ntc_table[0].temp_C;

    // Above table range
    if (R_ntc <= ntc_table[NTC_TABLE_SIZE - 1].resistance)
        return (float)ntc_table[NTC_TABLE_SIZE - 1].temp_C;

    // Find surrounding points
    for (uint16_t i = 0; i < NTC_TABLE_SIZE - 1; i++) {
        if (R_ntc <= ntc_table[i].resistance &&
            R_ntc >= ntc_table[i+1].resistance) {

            // Linear interpolation between two points
            float R_high = (float)ntc_table[i].resistance;
            float R_low  = (float)ntc_table[i+1].resistance;
            float T_high = (float)ntc_table[i].temp_C;
            float T_low  = (float)ntc_table[i+1].temp_C;

            float temp = T_high + (T_low - T_high) *
                         (R_high - R_ntc) / (R_high - R_low);
            return temp;
        }
    }

    return -273.15f;  // should never reach here
}


void convertAllTemperatures(void) {
    for (uint8_t s = 0; s < SLAVEBOARDS; s++) {
        for (uint8_t g = 0; g < NUM_GPIOS; g++) {
            temperatures[s][g] = convertNTCtoTemp(GpioReadings[s][g], TsrefReadings[s]);
        }
    }
}
