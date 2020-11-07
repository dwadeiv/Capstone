/**
 * @file sleep_routines.c
 * @author David Wade
 * @date February 10th, 2020
 * @brief Contains all the functions for the sleep routines
 *
 */

/*************************************************************************
 *
 * @file sleep.c
 **************************************************************************
 *
 * @section License
 * <b>(C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
 **************************************************************************
 *
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * 	  claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * 	  misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ***************************************************************************/


//***********************************************************************************
// Include files
//***********************************************************************************
#include "sleep_routines.h"


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************
static int lowest_energy_mode [MAX_ENERGY_MODES];


//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * 	Initializes the sleep routine
 *
 * @details
 * 	Initializes the array lowest_energy_mode to all zeros
 *
 * @note
 * 	Function is only used to initialize
 *
 *
 ******************************************************************************/

void sleep_open(void){
	for(int i = 0; i < 5; i++){
		lowest_energy_mode[i] = 0;
	}
}

/***************************************************************************//**
 * @brief
 * 	Blocks a certain energy mode from being entered by a peripheral
 *
 * @details
 * 	In addition, the function asserts there is an even amount of blocks
 * 	to unblocks
 *
 * @note
 * 	Will work with any energy mode
 *
 * @param[in] Energy mode
 *
 ******************************************************************************/

void sleep_block_mode(uint32_t EM){
	lowest_energy_mode[EM] = BLOCKED;
	EFM_ASSERT (lowest_energy_mode[EM] < 10);
}

/***************************************************************************//**
 * @brief
 * 	Unblocks a certain energy mode from being entered by a peripheral
 *
 * @details
 * 	In addition, the function asserts there is an even amount of blocks
 * 	to unblocks by checking that the values in the array nevver go below zero
 *
 * @note
 * 	Will work with any energy mode
 *
 * @param[in] Energy mode
 *
 ******************************************************************************/

void sleep_unblock_mode(uint32_t EM){
	lowest_energy_mode[EM] = UNBLOCKED;
	EFM_ASSERT (lowest_energy_mode[EM] >= 0);
}

/***************************************************************************//**
 * @brief
 * 	This function enters the application into the desired energy mode
 *
 * @details
 * 	This function parses through the private array to see which modes are blocked, then prevents the
 * 	application from entering the blocked sleep modes
 *
 * @note
 * 	This function takes into account all of the energy modes
 *
 *
 ******************************************************************************/

void enter_sleep(void){
	if(lowest_energy_mode[EM0] > 0){
		return;
	}
	else if(lowest_energy_mode[EM1] > 0){
		return;
	}
	else if(lowest_energy_mode[EM2] > 0){
		EMU_EnterEM1();
		return;
	}
	else if(lowest_energy_mode[EM3] > 0){
		EMU_EnterEM2(true);
		return;
	}
	else{
		EMU_EnterEM3(true);
		return;
	}

}

/***************************************************************************//**
 * @brief
 * 	This function checks to see which energy modes are blocked
 *
 * @details
 * 	Parses through the energy mode array to check to see which modes are blocked, if blocked, then
 * 	the blocked modes number is returned, IF non are blocked, then return is MAX_ENERGY_MODES - 1
 * 	since the index starts at 0
 *
 * @note
 * 	Function is used to determine cycling between energy modes
 *
 * @return
 * 	Current lowest energy mode
 *
 ******************************************************************************/

uint32_t current_block_energy_mode(void){
	int i = 0;
	while(i < MAX_ENERGY_MODES){
		if(lowest_energy_mode[i] != 0){
			return i;
		}
		else{
			i++;
		}
	}
	return(MAX_ENERGY_MODES - 1);
}






