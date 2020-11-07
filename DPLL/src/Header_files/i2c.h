/*
 * i2c.h
 *
 *  Created on: Feb 20, 2020
 *      Author: dwadeiv
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef SRC_HEADER_FILES_I2C_H_
#define SRC_HEADER_FILES_I2C_H_

#include <stdint.h>
#include "em_assert.h"
#include <stdbool.h>
#include "em_i2c.h"
#include "em_cmu.h"
#include "sleep_routines.h"
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************

//***********************************************************************************
// global variables
//***********************************************************************************

typedef enum{
	start,
	master_cmd,
	request_data,
	handle_data,
	stop

}eI2C_States;

typedef struct {
	bool					enable;
	uint32_t				freq;
	bool					master;
	uint32_t				refFreq;
	I2C_ClockHLR_TypeDef	clhr;
	uint32_t				sda_route;
	uint32_t				scl_route;
	uint32_t				sda_enable;
	uint32_t				scl_enable;
}I2C_OPEN_STRUCT;

typedef struct {
	uint8_t		sda_pin;
	uint8_t		scl_pin;
	uint8_t		sda_port;
	uint8_t		scl_port;

}I2C_IO_STRUCT;


typedef struct {
	eI2C_States 	state;
	I2C_TypeDef 	*i2c;
	uint32_t 		device_address;
	uint32_t		device_register;
	uint32_t		*data;
	int				bytes;
	bool			busy;
	bool			read;
	bool 			write;
	uint32_t		event;

}I2C_PAYLOAD;

#define I2C_EM_BLOCK 		EM2

#define I2C_ADDR_W(x)		(x << 1) & ~(0x01)
#define I2C_ADDR_R(x)		(x << 1) | (0x01)




//***********************************************************************************
// function prototypes
//***********************************************************************************
bool i2c_test();
void i2c_open(I2C_TypeDef *i2c, I2C_OPEN_STRUCT *i2c_setup, I2C_IO_STRUCT *i2c_reset);
void i2c_bus_reset();
bool i2c_busy(I2C_TypeDef *i2c);
void I2C0_IRQHandler();
void I2C1_IRQHandler();
void i2c_start(I2C_TypeDef *i2c, uint32_t device_address, uint32_t device_register, uint32_t *data, int bytes, bool read, uint32_t event);

#endif /* SRC_HEADER_FILES_I2C_H_ */


