#ifndef BQ79616_H
#define BQ79616_H

#include <stdint.h>

/* ------------------------------------
     User defined parameters
   ------------------------------------
*/
#define MAXBYTES (16*2)     //maximum number of bytes to be read from the devices (for array creation)
#define BAUDRATE 1000000    //device + uC baudrate

#define FRMWRT_SGL_R	0x00    //single device READ
#define FRMWRT_SGL_W	0x10    //single device WRITE
#define FRMWRT_STK_R	0x20    //stack READ
#define FRMWRT_STK_W	0x30    //stack WRITE
#define FRMWRT_ALL_R	0x40    //broadcast READ
#define FRMWRT_ALL_W	0x50    //broadcast WRITE
#define FRMWRT_REV_ALL_W 0x60   //broadcast WRITE reverse direction

#define VLSB_GPIO       152.59  // 1 LSB in μV (replace with datasheet value)


   //0x02 to 0x0E: range from 2700 mV to 3000 mV  //0x12 to 0x1E: range from 3500 mV to 3800 mV
 #define OV_THR 0x02    //0x22 to 0x2E: range from 4175 mV to 4475 mV

#define UV_THR 0x00 		//0x00 to 0x26: range from 1200 mV to 3100 mV
#define OVUV_MODE 0b01	//round robin mode on all active cells


#define OT_THR 0b000		// default values
#define UT_THR 0b111 		//66% of TSERF voltage
#define OTCB_THR 0xf				//??????
#define	COOLOFF_TMP	0x0			//??????
#define OTUT_MODE 0b01	//round robin mode on all active thermistors

#define BALANCING_THRESHOLD 0.020  // 20mV threshold to start balancing
#define SOC_BALANCING_START 80     // Only balance when SOC > 80%
#define TEMP_LIMIT 45.0            // Stop balancing if temperature > 45�C

typedef enum
{
    BQ79616_GPIO_DISABLED                 = 0b000, // High-Z
    BQ79616_GPIO_ADC_OTUT_INPUT           = 0b001,
    BQ79616_GPIO_ADC_INPUT                = 0b010,
    BQ79616_GPIO_DIGITAL_INPUT            = 0b011,
    BQ79616_GPIO_OUTPUT_HIGH              = 0b100,
    BQ79616_GPIO_OUTPUT_LOW               = 0b101,
    BQ79616_GPIO_ADC_INPUT_PULLUP         = 0b110,
    BQ79616_GPIO_ADC_INPUT_PULLDOWN       = 0b111

} BQ79616_GPIO_Config_t;


/* ------------------------------------
     function protoypes
   ------------------------------------
*/
//delay function
void DELAY_init(void);
void DELAY_microseconds(uint16_t us);
void TIM2_Delay_ms(uint32_t delay_ms);

/* ------------------------------------
     1. ping functions
   ------------------------------------
*/
//1. wake up ping
void Wake79616(void);
//2. shutdown ping
void SD79616(void);
//3. sleep to active ping
void StA79616(void);
//4. comm clear ping
void HWRST79616(void);

void AutoAddress(void);

void AutoAddress_Ring(void);
/* ------------------------------------
     2. read and write functions
   ------------------------------------
*/
int writeReg(uint8_t bID, uint16_t wAddr, uint64_t dwData, uint8_t bLen, uint8_t bWriteType);
int writeFrame(uint8_t bID, uint16_t wAddr, uint8_t * pData, uint8_t bLen, uint8_t bWriteType);
int readReg(uint8_t bID, uint16_t wAddr, uint8_t * pData, uint8_t bLen, uint32_t dwTimeOut, uint8_t bWriteType);
int readFrameReq(uint8_t bID, uint16_t wAddr, uint8_t bByteToReturn, uint8_t bWriteType);
uint16_t CRC16(uint8_t *pBuf, int nLen);
void RunCB();
float readGPIOVoltage(uint8_t BID, uint8_t GPIO_NUM, uint16_t* raw_value_ptr);

/* ------------------------------------
     3. Configurations functions
   ------------------------------------
*/

uint8_t configure_OTUT(uint8_t dev_address, uint8_t activeThermistors);
uint8_t configureGPIO(uint8_t GPIO_NUM, BQ79616_GPIO_Config_t GPIO_MODE ,uint8_t BID, uint8_t bWriteType);
void nfault_enable();
void ResetAllFaults(uint8_t bID, uint8_t bWriteType);
void MaskAllFaults(uint8_t bID, uint8_t bWriteType);

/* ------------------------------------
     BQ79616 Register Definitions
   ------------------------------------
*/

// OTP Shadow Registers (Non-volatile Memory)

#define BQ79616_DIR0_ADDR_OTP        0x00  // Stores device communication address
#define BQ79616_DIR1_ADDR_OTP        0x01  // Stores backup communication address
#define BQ79616_DEV_CONF             0x02  // Configuration for communication, fault detection, and operation modes
#define BQ79616_ACTIVE_CELL          0x03  // Specifies the number of active battery cells
#define BQ79616_OTP_SPARE15          0x04  // Reserved spare register
#define BQ79616_BBVC_POSN1           0x05  // Defines cell selection for voltage monitoring (cells 9-16)
#define BQ79616_BBVC_POSN2           0x06  // Defines cell selection for voltage monitoring (cells 1-8)
#define BQ79616_ADC_CONF1            0x07  // Configures auxiliary ADC settling time and low-pass filters
#define BQ79616_ADC_CONF2            0x08  // Configures ADC delay and other spare settings
#define BQ79616_OV_THRESH            0x09  // Sets overvoltage threshold levels
#define BQ79616_UV_THRESH            0x0A  // Sets undervoltage threshold levels
#define BQ79616_OTUT_THRESH          0x0B  // Defines over-temperature and under-temperature thresholds
#define BQ79616_UV_DISABLE1          0x0C  // Controls undervoltage disable for cells 9-16
#define BQ79616_UV_DISABLE2          0x0D  // Controls undervoltage disable for cells 1-8
#define BQ79616_GPIO_CONF1           0x0E  // Configures GPIO settings and fault input enable
#define BQ79616_GPIO_CONF2           0x0F  // Configures additional GPIO settings
#define BQ79616_GPIO_CONF3           0x10  // Configures GPIO 5 and 6
#define BQ79616_GPIO_CONF4           0x11  // Configures GPIO 7 and 8
#define BQ79616_FAULT_MSK1           0x16  // Configures fault mask settings for power, protection, and system faults
#define BQ79616_FAULT_MSK2           0x17  // Configures additional fault mask settings
#define BQ79616_PWR_TRANSIT_CONF     0x18  // Configures power transition behavior
#define BQ79616_COMM_TIMEOUT_CONF    0x19  // Sets communication timeout values
#define BQ79616_TX_HOLD_OFF          0x1A  // Configures transmission delay settings
#define BQ79616_MAIN_ADC_CAL1        0x1B  // Stores calibration values for the main ADC (low gain)
#define BQ79616_MAIN_ADC_CAL2        0x1C  // Stores calibration values for the main ADC (high gain and offset)
#define BQ79616_AUX_ADC_CAL1         0x1D  // Stores calibration values for auxiliary ADC (low gain)
#define BQ79616_AUX_ADC_CAL2         0x1E  // Stores calibration values for auxiliary ADC (high gain and offset)
#define BQ79616_CUST_CRC_HI          0x36  // Stores high byte of custom CRC
#define BQ79616_CUST_CRC_LO          0x37  // Stores low byte of custom CRC


// Read/Write Registers

#define BQ79616_OTP_PROG_UNLOCK1A    0x300  // Unlocks OTP programming mode
#define BQ79616_OTP_PROG_UNLOCK1B    0x301  // Unlocks OTP programming mode
#define BQ79616_OTP_PROG_UNLOCK1C    0x302  // Unlocks OTP programming mode
#define BQ79616_OTP_PROG_UNLOCK1D    0x303  // Unlocks OTP programming mode
#define BQ79616_DIR0_ADDR            0x306  // Sets communication address
#define BQ79616_DIR1_ADDR            0x307  // Sets backup communication address
#define BQ79616_COMM_CTRL            0x308  // Controls communication stack behavior
#define BQ79616_CONTROL1             0x309  // Controls sleep, shutdown, and reset behavior
#define BQ79616_CONTROL2             0x30A  // Enables hardware reset and reference voltage settings
#define BQ79616_OTP_PROG_CTRL        0x30B  // Controls OTP programming operations
#define BQ79616_ADC_CTRL1            0x30D  // Configures ADC low-pass filters and main ADC mode
#define BQ79616_ADC_CTRL2            0x30E  // Aligns auxiliary cell readings and selects channels
#define BQ79616_ADC_CTRL3            0x30F  // Selects auxiliary GPIO channels and ADC operation mode
#define BQ79616_CB_CELL_CTRL_16      0x318  // Controls cell balancing timing (cell 16)
#define BQ79616_CB_CELL_CTRL_15      0x319  // Controls cell balancing timing (cell 15)
#define BQ79616_CB_CELL_CTRL_14      0x31A  // Controls cell balancing timing (cell 14)
#define BQ79616_CB_CELL_CTRL_13      0x31B  // Controls cell balancing timing (cell 13)
#define BQ79616_CB_CELL_CTRL_12      0x31C  // Controls cell balancing timing (cell 12)
#define BQ79616_CB_CELL_CTRL_11      0x31D  // Controls cell balancing timing (cell 11)
#define BQ79616_CB_CELL_CTRL_10      0x31E  // Controls cell balancing timing (cell 10)
#define BQ79616_CB_CELL_CTRL_09      0x31F  // Controls cell balancing timing (cell 09)
#define BQ79616_CB_CELL_CTRL_08      0x320  // Controls cell balancing timing (cell 08)
#define BQ79616_CB_CELL_CTRL_07      0x321  // Controls cell balancing timing (cell 07)
#define BQ79616_CB_CELL_CTRL_06      0x322  // Controls cell balancing timing (cell 06)
#define BQ79616_CB_CELL_CTRL_05      0x323  // Controls cell balancing timing (cell 05)
#define BQ79616_CB_CELL_CTRL_04      0x324  // Controls cell balancing timing (cell 04)
#define BQ79616_CB_CELL_CTRL_03      0x325  // Controls cell balancing timing (cell 03)
#define BQ79616_CB_CELL_CTRL_02      0x326  // Controls cell balancing timing (cell 02)
#define BQ79616_CB_CELL_CTRL_01      0x327  // Controls cell balancing timing (cell 01)
#define BQ79616_VMB_DONE_THRESH      0x328  // Defines threshold for voltage balancing
#define BQ79616_MB_TIMER_CTRL        0x329  // Configures main balancing timer
#define BQ79616_VCB_DONE_THRESH      0x32A  // Defines threshold for cell balancing
#define BQ79616_OTCB_THRESH          0x32B  // Sets thermal cutoff threshold for cell balancing
#define BQ79616_OVUV_CTRL            0x32C  // Controls overvoltage/undervoltage thresholds and mode
#define BQ79616_OTUT_CTRL            0x32D  // Controls overtemperature/undertemperature thresholds and mode
#define BQ79616_BAL_CTRL1            0x32E  // Configures balance duty cycle
#define BQ79616_BAL_CTRL2            0x32F  // Enables cell balancing features
#define FAULT_RST2				     0x332
#define BAL_CTRL3				     0x330
#define FAULT_RST1				     0x331
#define DIAG_OTP_CTRL			     0x335
#define DIAG_COMM_CTRL			     0x336
#define DIAG_PWR_CTRL			     0x337
#define DIAG_CBFET_CTRL1		     0x338
#define DIAG_CBFET_CTRL2		     0x339
#define DIAG_COMP_CTRL1			     0x33A
#define DIAG_COMP_CTRL2			     0x33B
#define DIAG_COMP_CTRL3			     0x33C
#define DIAG_COMP_CTRL4			     0x33D
#define DIAG_PROT_CTRL			     0x33E
#define OTP_ECC_DATAIN1			     0x343
#define OTP_ECC_DATAIN2			     0x344
#define OTP_ECC_DATAIN3			     0x345
#define OTP_ECC_DATAIN4			     0x346
#define OTP_ECC_DATAIN5			     0x347
#define OTP_ECC_DATAIN6			     0x348
#define OTP_ECC_DATAIN7			     0x349
#define OTP_ECC_DATAIN8			     0x34A
#define OTP_ECC_DATAIN9			     0x34B
#define OTP_ECC_TEST			     0x34C
#define SPI_CONF				     0x34D
#define SPI_TX3					     0x34E
#define SPI_TX2					     0x34F
#define SPI_TX1					     0x350
#define SPI_EXE					     0x351
#define OTP_PROG_UNLOCK2A		     0x352
#define OTP_PROG_UNLOCK2B		     0x353
#define OTP_PROG_UNLOCK2C		     0x354
#define OTP_PROG_UNLOCK2D		     0x355
#define DEBUG_CTRL_UNLOCK		     0x700
#define DEBUG_COMM_CTRL1		     0x701
#define DEBUG_COMM_CTRL2		     0x702

// read only registers
#define PARTID					     0x500
#define DIE_ID1					     0x501
#define DIE_ID2					     0x502
#define DIE_ID3					     0x503
#define DIE_ID4					     0x504
#define DIE_ID5					     0x505
#define DIE_ID6					     0x506
#define DIE_ID7					     0x507
#define DIE_ID8					     0x508
#define DIE_ID9					     0x509
#define CUST_CRC_RSLT_HI		     0x50C
#define CUST_CRC_RSLT_LO		     0x50D
#define OTP_ECC_DATAOUT1		     0x510
#define OTP_ECC_DATAOUT2		     0x511
#define OTP_ECC_DATAOUT3		     0x512
#define OTP_ECC_DATAOUT4		     0x513
#define OTP_ECC_DATAOUT5		     0x514
#define OTP_ECC_DATAOUT6		     0x515
#define OTP_ECC_DATAOUT7		     0x516
#define OTP_ECC_DATAOUT8		     0x517
#define OTP_ECC_DATAOUT9		     0x518
#define OTP_PROG_STAT			     0x519
#define OTP_CUST1_STAT			     0x51A
#define OTP_CUST2_STAT			     0x51B
#define SPI_RX3					     0x520
#define SPI_RX2					     0x521
#define SPI_RX1					     0x522
#define DIAG_STAT				     0x526
#define ADC_STAT1				     0x527
#define ADC_STAT2				     0x528
#define GPIO_STAT				     0x52A
#define BAL_STAT				     0x52B
#define DEV_STAT				     0x52C
#define FAULT_SUMMARY			     0x52D
#define FAULT_COMM1				     0x530
#define FAULT_COMM2				     0x531
#define FAULT_COMM3				     0x532
#define FAULT_OTP				     0x535
#define FAULT_SYS				     0x536
#define FAULT_PROT1				     0x53A
#define FAULT_PROT2				     0x53B
#define FAULT_OV1				     0x53C
#define FAULT_OV2				     0x53D
#define FAULT_UV1				     0x53E
#define FAULT_UV2				     0x53F
#define FAULT_OT				     0x540
#define FAULT_UT				     0x541
#define FAULT_COMP_GPIO			     0x543
#define FAULT_COMP_VCCB1		     0x545
#define FAULT_COMP_VCCB2		     0x546
#define FAULT_COMP_VCOW1		     0x548
#define FAULT_COMP_VCOW2		     0x549
#define FAULT_COMP_CBOW1		     0x54B
#define FAULT_COMP_CBOW2		     0x54C
#define FAULT_COMP_CBFET1		     0x54E
#define FAULT_COMP_CBFET2		     0x54F
#define FAULT_COMP_MISC			     0x550
#define FAULT_PWR1				     0x552
#define FAULT_PWR2				     0x553
#define FAULT_PWR3				     0x554
#define CB_COMPLETE1			     0x556
#define CB_COMPLETE2			     0x557
#define BAL_TIME				     0x558

#define BQ79616_CELL_VOLTAGE_BASE 0x568

#define VCELL16_HI				     0x568
#define VCELL16_LO				     0x569
#define VCELL15_HI				     0x56A
#define VCELL15_LO				     0x56B
#define VCELL14_HI				     0x56C
#define VCELL14_LO				     0x56D
#define VCELL13_HI				     0x56E
#define VCELL13_LO				     0x56F
#define VCELL12_HI				     0x570
#define VCELL12_LO				     0x571
#define VCELL11_HI				     0x572
#define VCELL11_LO				     0x573
#define VCELL10_HI				     0x574
#define VCELL10_LO				     0x575
#define VCELL9_HI				     0x576
#define VCELL9_LO				     0x577
#define VCELL8_HI				     0x578
#define VCELL8_LO				     0x579
#define VCELL7_HI				     0x57A
#define VCELL7_LO				     0x57B
#define VCELL6_HI				     0x57C
#define VCELL6_LO				     0x57D
#define VCELL5_HI				     0x57E
#define VCELL5_LO				     0x57F
#define VCELL4_HI				     0x580
#define VCELL4_LO				     0x581
#define VCELL3_HI				     0x582
#define VCELL3_LO				     0x583
#define VCELL2_HI				     0x584
#define VCELL2_LO				     0x585
#define VCELL1_HI				     0x586
#define VCELL1_LO				     0x587
#define BUSBAR_HI				     0x588
#define BUSBAR_LO				     0x589
#define TSREF_HI				     0x58C
#define TSREF_LO				     0x58D
#define GPIO1_HI				     0x58E
#define GPIO1_LO				     0x58F
#define GPIO2_HI				     0x590
#define GPIO2_LO				     0x591
#define GPIO3_HI				     0x592
#define GPIO3_LO				     0x593
#define GPIO4_HI				     0x594
#define GPIO4_LO				     0x595
#define GPIO5_HI				     0x596
#define GPIO5_LO				     0x597
#define GPIO6_HI				     0x598
#define GPIO6_LO				     0x599
#define GPIO7_HI				     0x59A
#define GPIO7_LO				     0x59B
#define GPIO8_HI				     0x59C
#define GPIO8_LO				     0x59D
#define DIETEMP1_HI				     0x5AE
#define DIETEMP1_LO				     0x5AF
#define DIETEMP2_HI				     0x5B0
#define DIETEMP2_LO				     0x5B1
#define AUX_CELL_HI				     0x5B2
#define AUX_CELL_LO				     0x5B3
#define AUX_GPIO_HI				     0x5B4
#define AUX_GPIO_LO				     0x5B5
#define AUX_BAT_HI				     0x5B6
#define AUX_BAT_LO				     0x5B7
#define AUX_REFL_HI				     0x5B8
#define AUX_REFL_LO				     0x5B9
#define AUX_VBG2_HI				     0x5BA
#define AUX_VBG2_LO				     0x5BB
#define AUX_VREF4P2_HI			     0x5BC
#define AUX_VREF4P2_LO			     0x5BD
#define AUX_AVAO_REF_HI			     0x5BE
#define AUX_AVAO_REF_LO			     0x5BF
#define AUX_AVDD_REF_HI			     0x5C0
#define AUX_AVDD_REF_LO			     0x5C1
#define AUX_OV_DAC_HI			     0x5C2
#define AUX_OV_DAC_LO			     0x5C3
#define AUX_UV_DAC_HI			     0x5C4
#define AUX_UV_DAC_LO			     0x5C5
#define AUX_OT_OTCB_DAC_HI		     0x5C6
#define AUX_OT_OTCB_DAC_LO		     0x5C7
#define AUX_UT_DAC_HI			     0x5C8
#define AUX_UT_DAC_LO			     0x5C9
#define AUX_VCBDONE_DAC_HI		     0x5CA
#define AUX_VCBDONE_DAC_LO		     0x5CB
#define AUX_VCM1_HI				     0x5CC
#define AUX_VCM1_LO				     0x5CD
#define VREF4P2_HI				     0x5CE
#define VREF4P2_LO				     0x5CF
#define REFH_HI					     0x5D0
#define REFH_LO					     0x5D1
#define DIAG_MAIN_HI			     0x5D2
#define DIAG_MAIN_LO			     0x5D3
#define DIAG_AUX_HI				     0x5D4
#define DIAG_AUX_LO				     0x5D5
#define CURRENT_HI				     0x5D6
#define CURRENT_MID				     0x5D7
#define CURRENT_LO				     0x5D8
#define DEBUG_COMM_STAT			     0x780
#define DEBUG_UART_RC			     0x781
#define DEBUG_UART_RR_TR		     0x782
#define DEBUG_COMH_BIT			     0x783
#define DEBUG_COMH_RC			     0x784
#define DEBUG_COMH_RR_TR		     0x785
#define DEBUG_COML_BIT			     0x786
#define DEBUG_COML_RC			     0x787
#define DEBUG_COML_RR_TR		     0x788
#define DEBUG_UART_DISCARD		     0x789
#define DEBUG_COMH_DISCARD		     0x78A
#define DEBUG_COML_DISCARD		     0x78B
#define DEBUG_UART_VALID_HI		     0x78C
#define DEBUG_UART_VALID_LO		     0x78D
#define DEBUG_COMH_VALID_HI		     0x78E
#define DEBUG_COMH_VALID_LO		     0x78F
#define DEBUG_COML_VALID_HI		     0x790
#define DEBUG_COML_VALID_LO		     0x791
#define DEBUG_OTP_SEC_BLK		     0x7A0
#define DEBUG_OTP_DED_BLK		     0x7A1

#endif // BQ79616_H
