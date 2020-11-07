/*
 * SI7021.h
 *
 *  Created on: Feb 25, 2020
 *      Author: dwadeiv
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef SRC_HEADER_FILES_SI7021_H_
#define SRC_HEADER_FILES_SI7021_H_

// Un-comment to perform test
#define SI7021_I2C_TEST_ENABLED

// Device setup Macros
#define		Si7021_dev_addr				0x40 	// Si7021 I2C device address
#define		Si7021_TEMP_NO_HOLD			0xF3 	// Si7021 temp read/no hold cmd
#define		Si7021_I2C_FREQ				I2C_FREQ_FAST_MAX 	//400kHz
#define		Si7021_I2C_CLK_RATIO		i2cClockHLRAsymetric
#define		Si7021_SCL_LOC				_I2C_ROUTELOC0_SCLLOC_LOC0
#define		Si7021_SCL_EN				I2C_ROUTEPEN_SCLPEN
#define		Si7021_SDA_LOC				_I2C_ROUTELOC0_SDALOC_LOC0
#define		Si7021_SDA_EN				I2C_ROUTEPEN_SDAPEN
#define		Si7021_I2C					I2C0 	// The PG I2C peripheral to use
#define		Si7021_REF_FREQ 			0
#define		TEMP_READ_BYTES_COUNT		2

// Used to convert data
#define		Si7021_TEMP_CONVR			0.00268127
#define		Si7021_TEMP_SUB				46.85
#define		Si7021_RH_CONVR				0.0019073486
#define		Si7021_RH_SUB				6

// Hardware Delay
#define 	DELAY						80 // ms

// Used to change Si7021 resolution
#define		NO_EVENT					0
#define 	W_USER_1_REG				0xE6
#define 	R_USER_1_REG				0xE7
#define		Si7021_RH_NO_HOLD			0xF5 	// Si7021 humidity read/no hold cmd
#define 	USR_REG_READ_BYTES_COUNT	1
#define		Si7021_U1_DEFAULT			0x3A
#define 	Si7021_RES_MASK				0x81
#define		Si7021_RES_SET				0x80 //RES1 - 1 RES0 - 0 you have 13 bit
#define		Si7021_TARGET_U1			0xBA

// USed for testing
#define		TEST_TEMP_LOW_BOUND			60
#define		TEST_TEMP_HIGH_BOUND		100



#include "i2c.h"


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// global variables
//***********************************************************************************



//***********************************************************************************
// function prototypes
//***********************************************************************************
bool si7021_i2c_test();
void si7021_i2c_open();
void change_si7021_resolution();
void si7021_i2c_write(uint32_t write_command, uint8_t bytes);
void si7021_i2c_read(uint32_t read_command, uint8_t bytes, uint32_t event);
float si7021_i2c_temp_convert();
float si7021_i2c_rh_convert();

#endif /* SRC_HEADER_FILES_SI7021_H_ */

