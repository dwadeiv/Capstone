/*
 * sleep_rotutines.h
 *
 *  Created on: Feb 11, 2020
 *      Author: dwadeiv
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef SRC_HEADER_FILES_SLEEP_ROTUTINES_H_
#define	SRC_HEADER_FILES_SLEEP_ROTUTINES_H_

#include <stdint.h>
#include "em_emu.h"
#include "em_int.h"
#include "em_assert.h"
#include "em_core.h"
#include <stdbool.h>


#endif /* SRC_HEADER_FILES_SLEEP_ROTUTINES_H_ */

//***********************************************************************************
// defined files
//***********************************************************************************
#define 	EM0				0
#define 	EM1				1
#define 	EM2				2
#define 	EM3				3
#define 	EM4				4
#define	MAX_ENERGY_MODES	5

#define BLOCKED				1
#define UNBLOCKED			0


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void sleep_open(void);
void sleep_block_mode(uint32_t EM);
void sleep_unblock_mode(uint32_t EM);
void enter_sleep(void);
uint32_t current_block_energy_mode(void);

