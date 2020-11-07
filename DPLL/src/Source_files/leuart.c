/**
 * @file leuart.c
 * @author Keith Graham
 * @date November 27th, 2019
 * @brief Contains all the functions of the LEUART peripheral
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Library includes
#include <string.h>

//** Silicon Labs include files
#include "em_gpio.h"
#include "em_cmu.h"

//** Developer/user include files
#include "leuart.h"
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************
static uint32_t	rx_done_evt;
static uint32_t	tx_done_evt;
//static uint32_t	str_ptr;
//static uint32_t	max_len;
//static char		output_str[80];
//static bool		leuart0_tx_busy;

static LEUART_PAYLOAD payload;

/***************************************************************************//**
 * @brief LEUART driver
 * @details
 *  This module contains all the functions to support the driver's state
 *  machine to transmit a string of data across the LEUART bus.  There are
 *  additional functions to support the Test Driven Development test that
 *  is used to validate the basic set up of the LEUART peripheral.  The
 *  TDD test for this class assumes that the LEUART is connected to the HM-18
 *  BLE module.  These TDD support functions could be used for any TDD test
 *  to validate the correct setup of the LEUART.
 *
 ******************************************************************************/

//***********************************************************************************
// Private functions
//***********************************************************************************

static void leuart_txbl();
static void leuart_txc();


//***********************************************************************************
// Global functions
//***********************************************************************************



/***************************************************************************//**
 * @brief
 * 	This function performs the necessary steps to get LEUART ready for operation
 *
 * @details
 * 	It begins with enabling the necessary clocks for the leuart peripheral, testing leuart with the startframe register,
 * 	initializing parameters for communication, routing and enabling the tx and rx pins, enabling either tx or rx
 * 	depending on the application, then clearing the TXC interrupt and calling the IRQHandler.
 *
 * @note
 * 	This function will work with any leuart
 *
 * @param[in] leuart
 *  LEUART peripheral being used
 *
 * @param[in] leuart_settings
 * 	LEUART_OPEN_STRUCT that was initialized in ble.c
 *
 ******************************************************************************/

void leuart_open(LEUART_TypeDef *leuart, LEUART_OPEN_STRUCT *leuart_settings){
	LEUART_Init_TypeDef	leuart_values;
	if(leuart == LEUART0){
		CMU_ClockEnable(cmuClock_LEUART0 , true);
	}
	else{
		EFM_ASSERT(false);
	}
	// test
/////////////////////////////////////////////////////
	leuart->STARTFRAME = TEST_BITS;
	while(leuart->SYNCBUSY); // polling loop
	EFM_ASSERT(leuart->STARTFRAME == TEST_BITS);
	leuart->STARTFRAME = 0;
	while(leuart->SYNCBUSY);
	EFM_ASSERT(leuart->STARTFRAME == 0);
////////////////////////////////////////////////////
	// Initialize LEUART
	LEUART_Reset(leuart);
	leuart_values.baudrate = leuart_settings->baudrate;
	leuart_values.databits = leuart_settings->databits;
	leuart_values.enable = leuart_settings->enable;
	leuart_values.parity = leuart_settings->parity;
	leuart_values.refFreq = leuart_settings->ref_freq;
	leuart_values.stopbits = leuart_settings->stopbits;
	rx_done_evt = leuart_settings->rx_done_evt;
	tx_done_evt = leuart_settings->tx_done_evt;


	// Route the LEUART
	leuart->ROUTELOC0 = leuart_settings->rx_loc | leuart_settings->tx_loc;
	leuart->ROUTEPEN = (leuart_settings->rx_pin_en & LEUART_ROUTEPEN_RXPEN) | (leuart_settings->tx_pin_en & LEUART_ROUTEPEN_TXPEN);

	leuart->CMD = LEUART_CMD_CLEARTX | LEUART_CMD_CLEARRX;
	LEUART_Init(leuart, &leuart_values);
	while(leuart->SYNCBUSY);

	// TX is enabled
	if(leuart_settings->tx_en){
		leuart->CMD |= LEUART_CMD_TXEN;
		while(leuart->SYNCBUSY);
		while(!(leuart->STATUS & LEUART_STATUS_TXENS));
		EFM_ASSERT(leuart->STATUS & LEUART_STATUS_TXENS);
	}
	else{
		leuart->CMD &= ~LEUART_CMD_TXEN;
		EFM_ASSERT(!(leuart->STATUS & LEUART_STATUS_TXENS));
	}
	// RX is enabled
	if(leuart_settings->rx_en){
		leuart->CMD |= LEUART_CMD_RXEN;
		while(leuart->SYNCBUSY);
		while(!(leuart->STATUS & LEUART_STATUS_RXENS));
		EFM_ASSERT(leuart->STATUS & LEUART_STATUS_RXENS);
	}
	else{
		leuart->CMD &= ~LEUART_CMD_RXEN;
		EFM_ASSERT(!(leuart->STATUS & LEUART_STATUS_RXENS));
	}
	// Clear the TXC IF
	leuart->IFC = LEUART_IFC_TXC;
	if(leuart == LEUART0){
		NVIC_EnableIRQ(LEUART0_IRQn);
	}

	LEUART_Enable(leuart, leuart_settings->enable);
	while(leuart->SYNCBUSY);
}

/***************************************************************************//**
 * @brief
 * 	This is the IRQ Handler for LEUART0
 *
 * @details
 * 	Standard IRQHandler that clears the interrupt flag and then services the given interrupt
 *
 * @note
 * 	This function is for LEUAR0
 *
 ******************************************************************************/

void LEUART0_IRQHandler(void){
	uint32_t int_flag;
	int_flag = LEUART0->IF & LEUART0->IEN;
	LEUART0->IFC = int_flag;
	if(int_flag & LEUART_IF_TXC){
		leuart_txc();
	}
	if(int_flag & LEUART_IF_TXBL){
		leuart_txbl();
	}
}

/***************************************************************************//**
 * @brief
 * 	This function begins the state machine for leuart communication. This function alone switches
 * 	between the first two states of the state machine
 *
 * @details
 * 	Function starts by blocking the required energy mode, then making sure the LEUART
 * 	interrupts are cleared. Then it fills in the payload struct using the arguments passed.
 * 	Finally its enables the TXBL interrupt which begins the serial communication.
 *
 * @note
 * 	This function will work with any leuart or string
 *
 * @param[in] leuart
 *   LEUART peripheral being used
 *
 * @param[in] string
 * 	String that will be transmitted
 *
 * @param[in] string_len
 * 	Length of the string being transmitted
 *
 ******************************************************************************/

void leuart_start(LEUART_TypeDef *leuart, char *string, uint32_t string_len){
	// Block the sleep mode
	sleep_block_mode(LEUART_TX_EM);
	LEUART_IntClear(leuart, LEUART_IFC_TXC);
	// Fill in payload struct
	payload.busy = true;
	payload.state = LEUART_Start;
	payload.message_length = string_len;
	strcpy(payload.message, string);
	payload.leuart = leuart;
	payload.index = 0;
	payload.state = Transmit_data;

	LEUART_IntEnable(leuart, LEUART_IEN_TXBL);

}

/***************************************************************************//**
 * @brief
 * 	This function returns whether or not the leuart is transmitting data
 *
 * @details
 * 	Returns the payload.busy variable
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns whether or not data is being transmitted
 *
 ******************************************************************************/

bool leuart_tx_busy(LEUART_TypeDef *leuart){

	return payload.busy;

}

/***************************************************************************//**
 * @brief
 *   LEUART STATUS function returns the STATUS of the peripheral for the
 *   TDD test
 *
 * @details
 * 	 This function enables the LEUART STATUS register to be provided to
 * 	 a function outside this .c module.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns the STATUS register value as an uint32_t value
 *
 ******************************************************************************/

uint32_t leuart_status(LEUART_TypeDef *leuart){
	uint32_t	status_reg;
	status_reg = leuart->STATUS;
	return status_reg;
}

/***************************************************************************//**
 * @brief
 *   LEUART CMD Write sends a command to the CMD register
 *
 * @details
 * 	 This function is used by the TDD test function to program the LEUART
 * 	 for the TDD tests.
 *
 * @note
 *   Before exiting this function to update  the CMD register, it must
 *   perform a SYNCBUSY while loop to ensure that the CMD has by synchronized
 *   to the lower frequency LEUART domain.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @param[in] cmd_update
 * 	 The value to write into the CMD register
 *
 ******************************************************************************/

void leuart_cmd_write(LEUART_TypeDef *leuart, uint32_t cmd_update){

	leuart->CMD = cmd_update;
	while(leuart->SYNCBUSY);
}

/***************************************************************************//**
 * @brief
 *   LEUART IF Reset resets all interrupt flag bits that can be cleared
 *   through the Interrupt Flag Clear register
 *
 * @details
 * 	 This function is used by the TDD test program to clear interrupts before
 * 	 the TDD tests and to reset the LEUART interrupts before the TDD
 * 	 exits
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 ******************************************************************************/

void leuart_if_reset(LEUART_TypeDef *leuart){
	leuart->IFC = 0xffffffff;
}

/***************************************************************************//**
 * @brief
 *   LEUART App Transmit Byte transmits a byte for the LEUART TDD test
 *
 * @details
 * 	 The BLE module will respond to AT commands if the BLE module is not
 * 	 connected to the phone app.  To validate the minimal functionality
 * 	 of the LEUART peripheral, write and reads to the LEUART will be
 * 	 performed by polling and not interrupts.
 *
 * @note
 *   In polling a transmit byte, a while statement checking for the TXBL
 *   bit in the Interrupt Flag register is required before writing the
 *   TXDATA register.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @param[in] data_out
 *   Byte to be transmitted by the LEUART peripheral
 *
 ******************************************************************************/

void leuart_app_transmit_byte(LEUART_TypeDef *leuart, uint8_t data_out){
	while (!(leuart->IF & LEUART_IF_TXBL));
	leuart->TXDATA = data_out;
}


/***************************************************************************//**
 * @brief
 *   LEUART App Receive Byte polls a receive byte for the LEUART TDD test
 *
 * @details
 * 	 The BLE module will respond to AT commands if the BLE module is not
 * 	 connected to the phone app.  To validate the minimal functionality
 * 	 of the LEUART peripheral, write and reads to the LEUART will be
 * 	 performed by polling and not interrupts.
 *
 * @note
 *   In polling a receive byte, a while statement checking for the RXDATAV
 *   bit in the Interrupt Flag register is required before reading the
 *   RXDATA register.
 *
 * @param[in] leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns the byte read from the LEUART peripheral
 *
 ******************************************************************************/

uint8_t leuart_app_receive_byte(LEUART_TypeDef *leuart){
	uint8_t leuart_data;
	while (!(leuart->IF & LEUART_IF_RXDATAV));
	leuart_data = leuart->RXDATA;
	return leuart_data;
}

/***************************************************************************//**
 * @brief
 * 	State machine for when a txbl interrupt occurs
 *
 * @details
 *	This function acts as a state machine for the txbl interupt and asserts false for all
 *	states that aren't supposed to have a txbl. It transmits the message character by character
 *	until it reaches the second to last character. This is to prevent an extra txbl interrupt
 *	taking place. Once the second to last character is reached, then it goes into the other conditional
 *	where the last character is transmitted, switches to the next state, disables the TXBL interrupt,
 *	and enables the txc interrupt.
 *
 * @note
 * 	Function is called by the IRQHandler when the txbl interrupt occurs
 *
 ******************************************************************************/
static void leuart_txbl(){
	switch(payload.state){
		case(LEUART_Start):{
			EFM_ASSERT(false);
			break;
		}
		case(Transmit_data):{
			// Message length - 1 hasn't been copied
			if(payload.index < (payload.message_length - 1)){
				LEUART0->TXDATA = payload.message[payload.index];
				payload.index ++;
			}
			// Last character is copied and the TXC is enabled
			else{
				LEUART0->TXDATA = payload.message[payload.index];
				payload.state = Wait_finish;
				LEUART_IntDisable(payload.leuart, LEUART_IEN_TXBL);
				LEUART_IntEnable(payload.leuart, LEUART_IEN_TXC);
			}
			break;
		}
		case(Wait_finish):{
			EFM_ASSERT(false);
			break;
		}
		case(Transmission_end):{
			EFM_ASSERT(false);
			break;
		}
		default:{
			EFM_ASSERT(false);
			break;
		}
	}
}

/***************************************************************************//**
 * @brief
 * 	State machine for when a txc interrupt occurs
 *
 * @details
 *	This function acts as a state machine for the txc interrupt and asserts false for all
 *	states that aren't supposed to have a txc. It switches to the last state of the state machine,
 *	disables the txc interrupt. unblocks the blocked sleep mode. and adds the tx_done_evt to the
 *	scheduler.
 *
 * @note
 * 	Function is called by the IRQHandler when the txc interrupt occurs
 *
 ******************************************************************************/
static void leuart_txc(){
	switch(payload.state){
		case(LEUART_Start):{
			EFM_ASSERT(false);
			break;
		}
		case(Transmit_data):{
			EFM_ASSERT(false);
			break;
		}
		case(Wait_finish):{
			// End of LEUART transmission. SLeep mode is unblocked and tx_done_evt is added to the scheduler
			payload.state = Transmission_end;
			LEUART_IntDisable(payload.leuart, LEUART_IEN_TXC);
			sleep_unblock_mode(LEUART_TX_EM);
			payload.busy = false;
			add_scheduled_event(tx_done_evt);
			break;
		}
		case(Transmission_end):{
			EFM_ASSERT(false);
			break;
		}
		default:{
			EFM_ASSERT(false);
			break;
		}
	}
}

