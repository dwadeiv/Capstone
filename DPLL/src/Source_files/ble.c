/**
 * @file ble.c
 * @author Keith Graham
 * @date November 28th, 2019
 * @brief Contains all the functions to interface the application with the HM-18
 *   BLE module and the LEUART driver
 *
 */

// https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

//***********************************************************************************
// Include files
//***********************************************************************************
#include "ble.h"
#include <string.h>

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************

CIRC_TEST_STRUCT test_struct;
BLE_CIRCULAR_BUF ble_cbuf;

static uint8_t ble_circ_space(void);
static void update_circ_wrtindex(BLE_CIRCULAR_BUF*index_struct, uint32_t update_by);
static void update_circ_readindex(BLE_CIRCULAR_BUF*index_struct, uint32_t update_by);

/***************************************************************************//**
 * @brief BLE module
 * @details
 *  This module contains all the functions to interface the application layer
 *  with the HM-18 Bluetooth module.  The application does not have the
 *  responsibility of knowing the physical resources required, how to
 *  configure, or interface to the Bluetooth resource including the LEUART
 *  driver that communicates with the HM-18 BLE module.
 *
 ******************************************************************************/

//***********************************************************************************
// Private functions
//***********************************************************************************


//***********************************************************************************
// Global functions
//***********************************************************************************


/***************************************************************************//**
 * @brief
 * 	This function initializes the LEUART_OPEN_STRUCT for serial communication
 *
 * @details
 * 	This function initializes all the necessary information for the LEUART driver
 * 	to be able to communicate with a BLE device then calls the leuart_open function
 * 	passing the initialized information
 *
 * @note
 * 	All of the macros can remain the same in this function, if one would want to use
 * 	a different BLE, they would just have to edit the values in the header file
 *
 * @param[in] tx_event
 *   Value of the tx_event for the scheduler
 *
 * @param[in] rx_event
 * 	Value of the rx_event for the scheduler
 *
 ******************************************************************************/

void ble_open(uint32_t tx_event, uint32_t rx_event){
	LEUART_OPEN_STRUCT ble_values;
	ble_values.baudrate = HM18_BAUDRATE;
	ble_values.databits = HM18_DATABITS;
	ble_values.enable = HM18_ENABLE;
	ble_values.parity = HM18_PARITY;
	ble_values.ref_freq = HM18_REFFREQ;
	ble_values.rx_loc = LEUART0_RX_ROUTE;
	ble_values.rx_en = LEUART0_RX_EN;
	ble_values.rx_pin_en = LEUART0_RX_PIN_EN;
	ble_values.tx_loc = LEUART0_TX_ROUTE;
	ble_values.tx_en = LEUART0_TX_EN;
	ble_values.tx_pin_en = LEUART0_TX_PIN_EN;
	ble_values.stopbits = HM18_STOPBITS;
	ble_values.tx_done_evt = tx_event;
	ble_values.rx_done_evt = rx_event;

	leuart_open(HM18_LEUART0, &ble_values);
	// Initializes the circular buffer
	ble_circ_init();
}

/***************************************************************************//**
 * @brief
 * 	This function writes the string argument to the BLE module
 *
 * @details
 * 	The function calls leuart_start function passing the LEUART being used, the
 * 	input string, and the length of the string
 *
 * @note
 * 	THis function will work with any LEUART and any string
 *
 * @param[in] string
 *   String that will be sent to the BLE
 *
 ******************************************************************************/

void ble_write(char* string){
	ble_circ_push(string);
	ble_circ_pop(false);
}

/***************************************************************************//**
 * @brief
 *   BLE Test performs two functions.  First, it is a Test Driven Development
 *   routine to verify that the LEUART is correctly configured to communicate
 *   with the BLE HM-18 module.  Second, the input argument passed to this
 *   function will be written into the BLE module and become the new name
 *   advertised by the module while it is looking to pair.
 *
 * @details
 * 	 This global function will use polling functions provided by the LEUART
 * 	 driver for both transmit and receive to validate communications with
 * 	 the HM-18 BLE module.  For the assignment, the communication with the
 * 	 BLE module must use low energy design principles of being an interrupt
 * 	 driven state machine.
 *
 * @note
 *   For this test to run to completion, the phone most not be paired with
 *   the BLE module.  In addition for the name to be stored into the module
 *   a breakpoint must be placed at the end of the test routine and stopped
 *   at this breakpoint while in the debugger for a minimum of 5 seconds.
 *
 * @param[in] *mod_name
 *   The name that will be written to the HM-18 BLE module to identify it
 *   while it is advertising over Bluetooth Low Energy.
 *
 * @return
 *   Returns bool true if successfully passed through the tests in this
 *   function.
 ******************************************************************************/

bool ble_test(char *mod_name){
	uint32_t	str_len;

	__disable_irq();
	// This test will limit the test to the proper setup of the LEUART
	// peripheral, routing of the signals to the proper pins, pin
	// configuration, and transmit/reception verification.  The test
	// will communicate with the BLE module using polling routines
	// instead of interrupts.
	//
	// How is polling different than using interrupts?
	// ANSWER: In interrupt, the device notifies the CPU that it needs servicing whereas,
	// in polling CPU repeatedly checks whether a device needs servicing.
	//
	// How does interrupts benefit the system for low energy operation?
	// ANSWER: It takes less energy due to the CPU only checking when an interrupt flag
	// is triggered whereas with polling, the CPU is consuming more energy by constantly
	// checking.
	//
	// How does interrupts benefit the system that has multiple tasks?
	// ANSWER: Because you service a single interrupt at a time which will take less
	// processing power.

	// First, you will need to review the DSD HM10 datasheet to determine
	// what the default strings to write data to the BLE module and the
	// expected return statement from the BLE module to test / verify the
	// correct response

	// The break_str is used to tell the BLE module to end a Bluetooth connection
	// such as with your phone.  The ok_str is the result sent from the BLE module
	// to the micro-controller if there was not active BLE connection at the time
	// the break command was sent to the BLE module.
	// Replace the break_str "" with the command to break or end a BLE connection
	// Replace the ok_str "" with the result that will be returned from the BLE
	//   module if there was no BLE connection
	char		break_str[80] = "AT";
	char		ok_str[80] = "OK";

	// output_str will be the string that will program a name to the BLE module.
	// From the DSD HM10 datasheet, what is the command to program a name into
	// the BLE module?
	// The  output_str will be a string concatenation of the DSD HM10 command
	// and the input argument sent to the ble_test() function
	// Replace the otput_str "" with the command to change the program name
	// Replace the result_str "" with the first part of the expected result
	//  the backend of the expected response will be concatenated with the
	//  input argument
	char		output_str[80] = "AT+NAME";
	char		result_str[80] = "OK+Set:";

	// To program the name into your module, you must reset the module after you
	// have sent the command to update the modules name.  What is the DSD HM10
	// name to reset the module?
	// Replace the reset_str "" with the command to reset the module
	// Replace the reset_result_str "" with the expected BLE module response to
	//  to the reset command
	char		reset_str[80] = "AT+RESET";
	char		reset_result_str[80] = "OK+RESET";
	char		return_str[80];

	bool		success;
	bool		rx_disabled, rx_en, tx_en;
	uint32_t	status;

	// These are the routines that will build up the entire command and response
	// of programming the name into the BLE module.  Concatenating the command or
	// response with the input argument name
	strcat(output_str, mod_name);
	strcat(result_str, mod_name);

	// The test routine must not alter the function of the configuration of the
	// LEUART driver, but requires certain functionality to insure the logical test
	// of writing and reading to the DSD HM10 module.  The following c-lines of code
	// save the current state of the LEUART driver that will be used later to
	// re-instate the LEUART configuration

	status = leuart_status(HM18_LEUART0);
	if (status & LEUART_STATUS_RXBLOCK) {
		rx_disabled = true;
		// Enabling, unblocking, the receiving of data from the LEUART RX port
		leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXBLOCKDIS);
	}
	else rx_disabled = false;
	if (status & LEUART_STATUS_RXENS) {
		rx_en = true;
	} else {
		rx_en = false;
		// Enabling the receiving of data from the RX port
		leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXEN);
		while (!(leuart_status(HM18_LEUART0) & LEUART_STATUS_RXENS));
	}

	if (status & LEUART_STATUS_TXENS){
		tx_en = true;
	} else {
		// Enabling the transmission of data to the TX port
		leuart_cmd_write(HM18_LEUART0, LEUART_CMD_TXEN);
		while (!(leuart_status(HM18_LEUART0) & LEUART_STATUS_TXENS));
		tx_en = false;
	}
//	leuart_cmd_write(HM10_LEUART0, (LEUART_CMD_CLEARRX | LEUART_CMD_CLEARTX));

	// This sequence of instructions is sending the break ble connection
	// to the DSD HM10 module.
	// Why is this command required if you want to change the name of the
	// DSD HM10 module?
	// ANSWER: Separate the module before changing it
	str_len = strlen(break_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM18_LEUART0, break_str[i]);
	}

	// What will the ble module response back to this command if there is
	// a current ble connection?
	// ANSWER: OK
	str_len = strlen(ok_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM18_LEUART0);
		if (ok_str[i] != return_str[i]) {
				EFM_ASSERT(false);;
		}
	}

	// This sequence of code will be writing or programming the name of
	// the module to the DSD HM10
	str_len = strlen(output_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM18_LEUART0, output_str[i]);
	}

	// Here will be the check on the response back from the DSD HM10 on the
	// programming of its name
	str_len = strlen(result_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM18_LEUART0);
		if (result_str[i] != return_str[i]) {
				EFM_ASSERT(false);;
		}
	}

	// It is now time to send the command to RESET the DSD HM10 module
	str_len = strlen(reset_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM18_LEUART0, reset_str[i]);
	}

	// After sending the command to RESET, the DSD HM10 will send a response
	// back to the micro-controller
	str_len = strlen(reset_result_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM18_LEUART0);
		if (reset_result_str[i] != return_str[i]) {
				EFM_ASSERT(false);;
		}
	}

	// After the test and programming have been completed, the original
	// state of the LEUART must be restored
	if (!rx_en) leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXDIS);
	if (rx_disabled) leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXBLOCKEN);
	if (!tx_en) leuart_cmd_write(HM18_LEUART0, LEUART_CMD_TXDIS);
	leuart_if_reset(HM18_LEUART0);

	success = true;

	__enable_irq();
	return success;
}

/***************************************************************************//**
 * @brief
 *   Circular Buff Test is a Test Driven Development function to validate
 *   that the circular buffer implementation
 *
 * @details
 *   This Test Driven Development test has tests integrated into the function
 *   to validate that the routines can successfully identify whether there
 *   is space available in the circular buffer, the write and index pointers
 *   wrap around, and that one or more packets can be pushed and popped from
 *   the circular buffer.
 *
 * @note
 *   If anyone of these test will fail, an EFM_ASSERT will occur.  If the
 *   DEBUG_EFM=1 symbol is defined for this project, exiting this function
 *   confirms that the push, pop, and the associated utility functions are
 *   working.
 *
 * @par
 *   There is a test escape that is not possible to test through this
 *   function that will need to be verified by writing several ble_write()s
 *   back to back and verified by checking that these ble_write()s were
 *   successfully transmitted to the phone app.
 *
 ******************************************************************************/
void circular_buff_test(void){
     bool buff_empty;
     int test1_len = 50;
     int test2_len = 25;
     int test3_len = 5;
     // Why this 0 initialize of read and write pointer?
     // Student Response: To indicate the buffer is empty
     //
     ble_cbuf.read_ptr = 0;
     ble_cbuf.write_ptr = 0;
     // Why do none of these test strings contain a 0?
     // Student Response: 0 represents a NULL character
     //
     for (int i = 0;i < test1_len; i++){
         test_struct.test_str[0][i] = i+1;
     }
     test_struct.test_str[0][test1_len] = 0;
     for (int i = 0;i < test2_len; i++){
         test_struct.test_str[1][i] = i + 20;
     }
     test_struct.test_str[1][test2_len] = 0;
     for (int i = 0;i < test3_len; i++){
         test_struct.test_str[2][i] = i +  35;
     }
     test_struct.test_str[2][test3_len] = 0;
     // What is this test validating?
     // Student response: It's making sure the buffer is empty
     EFM_ASSERT(ble_circ_space() == CSIZE);
     // Why is there only one push to the circular buffer at this stage of the test
     // Student Response: This is the simplest test for pushing anything to the circular
     // buffer.
     //
     ble_circ_push(&test_struct.test_str[0][0]);
     // What is this test validating?
     // Student response: It's making sure that the packet was properly pushed onto the buffer
     EFM_ASSERT(ble_circ_space() == (CSIZE - test1_len - 1));
     // Why is the expected buff_empty test = false?
     // Student Response: Pop returns false if a packet of data was properly popped off and true
     // if nothing was popped.
     //
     buff_empty = ble_circ_pop(CIRC_TEST);
     EFM_ASSERT(buff_empty == false);
     for (int i = 0; i < test1_len; i++){
         EFM_ASSERT(test_struct.test_str[0][i] == test_struct.result_str[i]);
     }
     // What is this test validating?
     // Student response: It's asserting that the date was placed with proper indexing for test_1
     EFM_ASSERT(test_struct.result_str[test1_len] == 0);
     // What is this test validating?
     // Student response: Its asserting the pop function worked and the buffer is empty
     EFM_ASSERT(ble_circ_space() == CSIZE);
     // What does this next push on the circular buffer test?
     // Student Response: If we can push data after popping
     ble_circ_push(&test_struct.test_str[1][0]);
     EFM_ASSERT(ble_circ_space() == (CSIZE - test2_len - 1));
     // What does this next push on the circular buffer test?
     // Student Response: Seeing if we can push more than a single character onto the buffer
     ble_circ_push(&test_struct.test_str[2][0]);
     EFM_ASSERT(ble_circ_space() == (CSIZE - test2_len - 1 - test3_len - 1));
     EFM_ASSERT(abs(ble_cbuf.write_ptr - ble_cbuf.read_ptr) < CSIZE);
     // Why is the expected buff_empty test = false?
     // Student Response: data was successfully popped off the buffer
     //
     buff_empty = ble_circ_pop(CIRC_TEST);
     EFM_ASSERT(buff_empty == false);
     for (int i = 0; i < test2_len; i++){
         EFM_ASSERT(test_struct.test_str[1][i] == test_struct.result_str[i]);
     }
     // What is this test validating?
     // Student response: It's asserting that the date was placed with proper indexing for test_2.
     EFM_ASSERT(test_struct.result_str[test2_len] == 0);
     EFM_ASSERT(ble_circ_space() == (CSIZE - test3_len - 1));
     // Why is the expected buff_empty test = false?
     // Student Response: data was successfully popped off the buffer
     //
     buff_empty = ble_circ_pop(CIRC_TEST);
     EFM_ASSERT(buff_empty == false);
     for (int i = 0; i < test3_len; i++){
         EFM_ASSERT(test_struct.test_str[2][i] == test_struct.result_str[i]);
     }
     // What is this test validating?
     // Student response: It's asserting that the date was placed with proper indexing for test_3.
     EFM_ASSERT(test_struct.result_str[test3_len] == 0);
     EFM_ASSERT(ble_circ_space() == CSIZE);
     // Using these three writes and pops to the circular buffer, what other test
     // could we develop to better test out the circular buffer?
     // Student Response: We could test pushing onto a full buffer
     // Why is the expected buff_empty test = true?
     // Student Response: There is no more data to pop from the buffer
     //
     buff_empty = ble_circ_pop(CIRC_TEST);
     EFM_ASSERT(buff_empty == true);
     ble_write("\nPassed Circular Buffer Test\n");
 }

/***************************************************************************//**
 * @brief
 * 	Initialization of the circular buffer
 *
 * @details
 * 	Sets read and write pointers to 0 and size of buffer to CSIZE
 *
 * @note
 * 	This is the payload for the circular buffer
 *
 ******************************************************************************/
void ble_circ_init(void){
	ble_cbuf.read_ptr = 0;
	ble_cbuf.write_ptr = 0;
	ble_cbuf.size = CSIZE;
	ble_cbuf.size_mask = CSIZE - 1;
}

/***************************************************************************//**
 * @brief
 * 	This method pushes packets of data onto the buffer
 *
 * @details
 * 	A header, that is equal to string length + 1, is placed at the beginning of the
 * 	packet being pushed. Then the string is pushed character by character.
 *
 * @note
 * 	The header is placed in order to tell the pop method how
 * 	long the string on the buffer is.
 *
 * @param[in] string
 * 	String being pushed onto the buffer
 *
 ******************************************************************************/
void ble_circ_push(char*string){
	int string_size = strlen(string);
	int space = ble_circ_space();
	int packet_length = string_size + 1;

	if(packet_length > space){
		EFM_ASSERT(false);
	}

	// placing header for pop method
	ble_cbuf.cbuf[ble_cbuf.write_ptr] = packet_length;
	update_circ_wrtindex(&ble_cbuf, 1);
	// placing string after header
	for(int i = 0; i < string_size; i++){
		ble_cbuf.cbuf[ble_cbuf.write_ptr] = string[i];
		update_circ_wrtindex(&ble_cbuf, 1);
	}
}

/***************************************************************************//**
 * @brief
 * 	This method pops a packet of data off of the buffer. Returns true if nothing was popped
 * 	and false if a string was popped
 *
 * @details
 * 	It first checks to see if the string is empty, then to see if data is currently being transmitted.
 * 	If both conditions are false, then it checks the read pointer for the header to see the string length,
 * 	then pops the entire string into a locally declared result_str, which is then passed to LEUART if
 * 	the test argument is false.
 *
 * @note
 * 	This method pops the data onto the buffer as well as sending it to LEUART
 *
 * @param[in] test
 * 	A boolean that tells if the data will be transmitted or not
 *
 * @return
 * 	 Returns true if no more data needs to be opped, false otherwise
 ******************************************************************************/
bool ble_circ_pop(bool test){

	if(ble_circ_space() == CSIZE){
		return true;
	}
	if(!leuart_tx_busy(LEUART0)){
		int string_length = ble_cbuf.cbuf[ble_cbuf.read_ptr] - 1;
		update_circ_readindex(&ble_cbuf, 1);

		// Copying buffer to local string
		for(int i = 0; i < string_length; i++){
			test_struct.result_str[i] = ble_cbuf.cbuf[ble_cbuf.read_ptr];
			update_circ_readindex(&ble_cbuf, 1);
		}

		// Adding a null character at the end of the string for the strlen function
		test_struct.result_str[string_length] = 0;
		// Transmitting the local string to LEUART
		if(!test){
			leuart_start(LEUART0, test_struct.result_str, strlen(test_struct.result_str));
		}
		return false;
	}
	return true;
}

/***************************************************************************//**
 * @brief
 * 	This method checks and returns the amount of free space in the circular buffer
 *
 * @details
 * 	This method uses the two's complement along with a bitwise and in order to obtain
 * 	the amount of space filled. Then it returns that result subtracted from the size of the
 * 	buffer.
 *
 * @note
 * 	There are less efficient ways to obtain the same result.
 *
 * @return
 * 	 Returns amount of free space
 ******************************************************************************/
uint8_t ble_circ_space(void){

	// Using 2s compliment arithmetic to obtain filled space
	uint8_t data_length = ((ble_cbuf.write_ptr - ble_cbuf.read_ptr) & (ble_cbuf.size_mask));
	// Free space = buffer size - filled space
	uint8_t free_space = ble_cbuf.size - data_length;

	return free_space;

}

/***************************************************************************//**
 * @brief
 * 	This function increments the write pointer
 *
 * @details
 * 	It increments the write pointer by the amount update_by. It ANDs it with size_mask
 * 	to handle roll over.
 *
 * @note
 * 	Bitwise AND is necessary so data isn't lost
 *
 * @param[in] index_struct
 * 	The circular buffer
 *
 * @param[in] update_by
 * 	The amount the pointer will be incremented
 *
 ******************************************************************************/
void update_circ_wrtindex(BLE_CIRCULAR_BUF*index_struct, uint32_t update_by){

	// Size mask is used to handle overflow
	index_struct->write_ptr = (index_struct->write_ptr + update_by) & index_struct->size_mask;

	return;
}

/***************************************************************************//**
 * @brief
 * 	This function increments the read pointer
 *
 * @details
 * 	It increments the write pointer by the amount update_by. It ANDs it with size_mask
 * 	to handle roll over.
 *
 * @note
 * 	Bitwise AND is necessary so data isn't lost
 *
 * @param[in] index_struct
 * 	The circular buffer
 *
 * @param[in] update_by
 * 	The amount the pointer will be incremented
 *
 ******************************************************************************/
void update_circ_readindex(BLE_CIRCULAR_BUF*index_struct, uint32_t update_by){

	// Size mask is used to handle overflow
	index_struct->read_ptr = (index_struct->read_ptr + update_by) & index_struct->size_mask;

	return;
}



