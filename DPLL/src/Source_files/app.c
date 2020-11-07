/**
 * @file app.c
 * @author David Wade
 * @date November 6th, 2020
 * @brief Contains the application code for the DPLL
 *
 */


//***********************************************************************************
// Include files
//***********************************************************************************
#include "app.h"
#include "letimer.h"


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// global variables
//***********************************************************************************
static char data_transmit[50];


//***********************************************************************************
// function
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * 	Used to initialize the peripherals
 *
 *
 * @details
 * 	Calls cmu_open to set up the ULFRCO clock to the Low Frequency clock tree
 * 	Calls gpio_open() to establish the output for the PWM
 * 	Calls app_letimer_pwm_open to create an instance if the pwm struct
 * 	Calls scheduler_open() to initialize the scheduler
 * 	Calls sleep_open to initialize the energy modes
 * 	Calls si7021_i2c_open to initialize readings from the temperature sensor
 * 	Adds BOOT_UP_EVT to the scheduler to test the BLE connection
 * 	Calls BLE open to initiate leuart communication
 *
 *
 * @note
 * 	Application code should not change anything from the LETIMER driver and the clock or GPIO configuration
 *
 *
 ******************************************************************************/
void app_peripheral_setup(void){
	cmu_open();
	gpio_open();
	app_letimer_pwm_open(PWM_PER, PWM_ACT_PER);
	scheduler_open();
	sleep_open();
	si7021_i2c_open();
	add_scheduled_event(BOOT_UP_EVT);
	ble_open(LEUART0_TX_DONE_EVT, LEUART0_RX_DONE_EVT);
}


/***************************************************************************//**
 * @brief
 * 	Creates instance of the letimer_pwm_struct
 *
 *
 * @details
 * 	Initializes all of the elements required for the struct
 *
 *
 * @note
 * 	Function should not change blueprint of the struct
 *
 *
 * @param[in] period
 * 	desired period for PWM
 *
 *
 * @param[in] active period
 * 	desired active period for the PWM
 *
 ******************************************************************************/
void app_letimer_pwm_open(float period, float act_period){
	// Initializing LETIMER0 for PWM operation by creating the
	// letimer_pwm_struct and initializing all of its elements
	APP_LETIMER_PWM_TypeDef letimer_pwm_struct;
	letimer_pwm_struct.debugRun = false;
	letimer_pwm_struct.enable = false;
	letimer_pwm_struct.out_pin_route0 = LETIMER0_ROUTE_OUT0;
	letimer_pwm_struct.out_pin_route1 = LETIMER0_ROUTE_OUT1;
	letimer_pwm_struct.out_pin_0_en = LETIMER0_OUT0_EN;
	letimer_pwm_struct.out_pin_1_en = LETIMER0_OUT1_EN;
	letimer_pwm_struct.period = period;
	letimer_pwm_struct.active_period = act_period;
	letimer_pwm_struct.uf_irq_enable = true;
	letimer_pwm_struct.uf_evt = LETIMER0_UF_EVT;
	letimer_pwm_struct.comp0_irq_enable = false;
	letimer_pwm_struct.comp0_evt = LETIMER0_COMP0_EVT;
	letimer_pwm_struct.comp1_irq_enable = false;
	letimer_pwm_struct.comp1_evt = LETIMER0_COMP1_EVT;


	letimer_pwm_open(LETIMER0, &letimer_pwm_struct);
}

/***************************************************************************//**
 * @brief
 * 	This function triggers a reading from the si7021 temperature sensor.
 *
 * @details
 * Initiates a temperature reading by calling the si7021_i2c_read() function
 *
 * @note
 * 	Function allows for continuous measurements of the temperature
 *
 ******************************************************************************/

void scheduled_letimer0_uf_evt (void){
	EFM_ASSERT(get_scheduled_event() & LETIMER0_UF_EVT);
	remove_scheduled_event(LETIMER0_UF_EVT);

	// Initiate humidity reading
	si7021_i2c_read(Si7021_RH_NO_HOLD, RH_READ_BYTES_COUNT, SI7021_RH_DONE_EVT);
}

/***************************************************************************//**
 * @brief
 * 	Removes scheduled COMP0 event
 *
 * @details
 * 	Function asserts that interrupts are disabled for COMP0
 *
 * @note
 * 	Event is still removed even though interrupts are disabled
 *
 *
 ******************************************************************************/

void scheduled_letimer0_comp0_evt (void){
	remove_scheduled_event(LETIMER0_COMP0_EVT);
	EFM_ASSERT(false);
}

/***************************************************************************//**
 * @brief
 * 	Removes scheduled COMP0 event
 *
 * @details
 * 	Function asserts that interrupts are disabled for COMP0
 *
 * @note
 * 	Event is still removed even though interrupts are disabled
 *
 *
 ******************************************************************************/

void scheduled_letimer0_comp1_evt (void){
	remove_scheduled_event(LETIMER0_COMP1_EVT);
	EFM_ASSERT(false);
}

/***************************************************************************//**
 * @brief
 * 	Notifies scheduler that a measurement has been taken and controls LED1. The
 * 	measurement is then prepares it to be transmitted to bluetooth.
 *
 * @details
 *	When the temperature is equal to or above THRESHOLD_TEMP, this function will turn
 *	LED1 on, otherwise LED1 will be off. After it appends the temp display sting to the end of
 *	the static variable data_transmit. It then calls ble_write() and passes data_transmit.
 *	Once the string is transmitted, the data_transmit string is cleared.
 *
 * @note
 * 	All the sensor data is transmitted when this scheduled event occurs.
 *
 ******************************************************************************/
void scheduled_si7021_temp_done_evt(void){
	remove_scheduled_event(SI7021_TEMP_DONE_EVT);

	float temp = si7021_i2c_temp_convert();

	// Control over LED1
	if(temp >= THRESHOLD_TEMP){
		GPIO_PinOutSet(LED1_port, LED1_pin);
	}
	else{
		GPIO_PinOutClear(LED1_port, LED1_pin);
	}

	// Preparing display string
	char temp_transmit[50];
	double number = (double)temp;
	sprintf(temp_transmit, "Temp = %4.1f F\n", number);

	// Append temp_transmit to end of data_transmit
	strcat(data_transmit,temp_transmit);

	ble_write(data_transmit);
	// Clear string
	strcpy(data_transmit,"");

}

/***************************************************************************//**
 * @brief
 * 	Notifies scheduler that a relative humidity measurement has been taken
 * 	and prepares it for bluetooth transmission.
 *
 * @details
 * 	After the humidity measurement is obtained, a display string is created and appended
 * 	to the data_transmit string. Then a temperature reading is initiated.
 *
 * @note
 * 	This scheduled event is initiated by the UF event
 *
 *
 ******************************************************************************/
void scheduled_si7021_rh_done_evt(void){
	remove_scheduled_event(SI7021_RH_DONE_EVT);

	float rh = si7021_i2c_rh_convert();

	// Preparing display string
	char rh_transmit[50];
	double number = (double)rh;
	sprintf(rh_transmit, "\nRelative Humidity = %4.1f%% \n", number);

	// Append rh_transmit to end of data_transmit
	strcat(data_transmit, rh_transmit);

	// Initiate temperature reading
	si7021_i2c_read(Si7021_TEMP_FROM_PREV_RH, TEMP_READ_BYTES_COUNT, SI7021_TEMP_DONE_EVT);

}

/***************************************************************************//**
 * @brief
 * 	This function is used to test the connection with the BLE module
 *
 * @details
 *  This test uses the basic HELLO World test. in addition if
 *  BLE_TEST_ENABLED is defined in ble.h, this function will perform the circular
 *  buffer test.
 *
 * @note
 * 	This function was used for test development
 *
 ******************************************************************************/
void scheduled_boot_up_evt(void){
	remove_scheduled_event(BOOT_UP_EVT);
	letimer_start(LETIMER0, true);

	#ifdef BLE_TEST_ENABLED
	circular_buff_test();
	// Un-comment following code to change name of BLE
//	bool test = ble_test("D-WADE's BLE");
//	EFM_ASSERT(test);
//	for (int i = 0; i < 20000000; i++);
	#endif

	#ifdef SI7021_I2C_TEST_ENABLED
	bool test = false;
	test = si7021_i2c_test();
	EFM_ASSERT(test);
	#endif

	// Method that changes the resolution of the Si7021
//	change_si7021_resolution();

	ble_write("\nHello World\n");
	ble_write("Course Project\n");
	ble_write("David Wade\n");
}

/***************************************************************************//**
 * @brief
 * 	This function is how the scheduler knows when the serial transmition is complete
 *
 * @details
 * 	This function removes the LEUART)_TX_DONE_EVT from the scheduler, the calls ble_circ_pop
 * 	with the argument false so the data can be sent to the leuart
 *
 ******************************************************************************/
void scheduled_leuart0_tx_done_evt(void){

	remove_scheduled_event(LEUART0_TX_DONE_EVT);
	ble_circ_pop(false);
}


