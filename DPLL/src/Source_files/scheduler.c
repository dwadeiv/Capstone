/**
 * @file scheduler.c
 * @author David Wade
 * @date February 11th, 2020
 * @brief Contains all the functions for the scheduler
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Libraries

//** Silicon Lab include files
#include "em_cmu.h"
#include "em_assert.h"
#include "em_int.h"

//** User/developer include files
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************
static unsigned int event_scheduled;


//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * 	Initializes the scheduler
 *
 * @details
 * 	Initializes the event_scheduled to 0
 *
 ******************************************************************************/

void scheduler_open(void){
	event_scheduled &= ~event_scheduled;
}

/***************************************************************************//**
 * @brief
 * 	This function adds an event to the scheduler
 *
 * @details
 * 	The function starts with disabling interrupts in order for the function to not be
 * 	interrupted, then makes event_scheduled equal to the parameter event. Then interrupts
 * 	are enabled again at the end of the function.
 *
 *
 * @note
 * 	This function is made for nested interrupts
 *
 * @param[in] event
 * 	The event that is being added to the scheduler
 *
 ******************************************************************************/

void add_scheduled_event(uint32_t event){
	__disable_irq();
	event_scheduled |= event;
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 * 	This function removes an event from the scheduler.
 *
 * @details
 * 	The function starts with disabling interrupts in order for the function to not be
 * 	interrupted, then makes event_scheduled not equal to the parameter event. Then interrupts
 * 	are enabled again at the end of the function.
 *
 * @note
 * 	This function is made for nested interrupts
 *
 * @param[in] event
 * The event that is being removed from the scheduler
 *
 ******************************************************************************/

void remove_scheduled_event(uint32_t event){
	__disable_irq();
	event_scheduled &= ~event;
	__enable_irq();

}

/***************************************************************************//**
 * @brief
 * 	Returns the current event_scheduled
 *
 * @details
 * 	Returns the current value of event_scheduled
 *
 * @note
 * 	Makes the private variable global for any file using this library
 *
 * @return
 * static unsigned int event_scheduled
 *
 ******************************************************************************/

uint32_t get_scheduled_event(void){
	return event_scheduled;
}


