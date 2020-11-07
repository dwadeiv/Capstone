/*
 * ble.h
 *
 *  Created on: May 22, 2019
 *      Author: xxx
 */

#ifndef SRC_HEADER_FILES_BLE_H_
#define SRC_HEADER_FILES_BLE_H_
//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "leuart.h"

//***********************************************************************************
// defined files
//***********************************************************************************

// for testing BLE Connection:
//#define BLE_TEST_ENABLED

#define HM18_LEUART0		LEUART0
#define HM18_BAUDRATE		9600
#define	HM18_DATABITS		leuartDatabits8
#define HM18_ENABLE			leuartEnable
#define HM18_PARITY			leuartNoParity
#define HM18_REFFREQ		0	// use reference clock
#define HM18_STOPBITS		leuartStopbits1

#define LEUART0_TX_ROUTE	LEUART_ROUTELOC0_TXLOC_LOC18
#define LEUART0_RX_ROUTE	LEUART_ROUTELOC0_RXLOC_LOC18

#define LEUART0_TX_PIN_EN	LEUART_ROUTEPEN_TXPEN
#define LEUART0_RX_PIN_EN	LEUART_ROUTEPEN_RXPEN

#define LEUART0_TX_EN		LEUART_CMD_TXEN
#define LEUART0_RX_EN		LEUART_CMD_RXEN

#define CIRC_TEST 		true
#define CIRC_OPER 		false
#define CSIZE 			64
#define CIRC_TEST_SIZE 	3

typedef struct{
	char 		cbuf[CSIZE];
	uint8_t 	size_mask; //CSIZE - 1
	uint32_t 	size;
	uint32_t 	read_ptr;
	uint32_t 	write_ptr;
} BLE_CIRCULAR_BUF;

typedef struct{
	char test_str[CIRC_TEST_SIZE][64];
	char result_str[64];
} CIRC_TEST_STRUCT;


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void ble_open(uint32_t tx_event, uint32_t rx_event);
void ble_write(char *string);
bool ble_test(char *mod_name);
void ble_circ_init(void);
void ble_circ_push(char*string);
void circular_buff_test(void);
bool ble_circ_pop(bool test);

#endif /* SRC_HEADER_FILES_BLE_H_ */
