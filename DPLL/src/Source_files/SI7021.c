/**
 * @file SI7021.c
 * @author David Wade
 * @date February 25th, 2020
 * @brief Contains all the functions for SI7021 temperature sensor
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "SI7021.h"
#include "HW_delay.h"
#include "gpio.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// global variables
//***********************************************************************************
static uint32_t write_data;
static uint32_t read_data;

//***********************************************************************************
// function prototypes
//***********************************************************************************


//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 * 	This test is to assert that you can write to the user 1 register in order to change the
 * 	resolution
 *
 * @details
 * 	This test first asserts that the user 1 register is in the default setting, then writes
 * 	to the register. The register is then checked for the expected value. The last test is to verify
 * 	that a reasonable temperature reading is still obtained by the sensor.
 *
 * @note
 * 	This test will change the temperature resolution to 13 bits.
 *
 * @return
 * 	 Returns whether or not test was passed
 ******************************************************************************/
bool si7021_i2c_test(){

	bool passed = false;

	// Making sure user 1 register is in default setting
	timer_delay(DELAY);
	si7021_i2c_read(R_USER_1_REG, USR_REG_READ_BYTES_COUNT, NO_EVENT);// USR_REG_READ_BYTES_COUNT
	while(i2c_busy(Si7021_I2C));
	EFM_ASSERT(read_data == Si7021_U1_DEFAULT); //use rx_data

	// Enabling 13 bit resolution in User 1 register
	write_data = read_data & ~Si7021_RES_MASK;// clearing target bits
	write_data |= Si7021_RES_SET; // setting target bits
	si7021_i2c_write(W_USER_1_REG, USR_REG_READ_BYTES_COUNT);
	while(i2c_busy(Si7021_I2C));
	timer_delay(DELAY);

	// Asserting 13 bit resolution is enabled
	si7021_i2c_read(R_USER_1_REG, USR_REG_READ_BYTES_COUNT, NO_EVENT);
	while(i2c_busy(Si7021_I2C));
	EFM_ASSERT(read_data == Si7021_TARGET_U1);

	// Asserting temperature output is in an expected range
	si7021_i2c_read(Si7021_TEMP_NO_HOLD, TEMP_READ_BYTES_COUNT, NO_EVENT);
	while(i2c_busy(Si7021_I2C));
	float test_temp = si7021_i2c_temp_convert();
	EFM_ASSERT(test_temp >= TEST_TEMP_LOW_BOUND && test_temp <= TEST_TEMP_HIGH_BOUND);

	passed = true;

	return passed;
}

/***************************************************************************//**
 * @brief
 * 	This function changes the resolution of the Si7021
 *
 * @details
 * 	The function changes the resolution to whatever Si7021_RES_SET is set to,
 * 	then asserts the register has the expected value of Si7021_TARGET_U1.
 *
 * @note
 * 	This method was not used for this particular lab, but could prove useful in other
 * 	applications.
 *
 ******************************************************************************/
void change_si7021_resolution(){

	timer_delay(DELAY);

	write_data = read_data & ~Si7021_RES_MASK;// clearing target bits
	write_data |= Si7021_RES_SET; // setting target bits
	si7021_i2c_write(W_USER_1_REG, USR_REG_READ_BYTES_COUNT);
	while(i2c_busy(Si7021_I2C));

	timer_delay(DELAY);

	si7021_i2c_read(R_USER_1_REG, USR_REG_READ_BYTES_COUNT, NO_EVENT);
	while(i2c_busy(Si7021_I2C));
	EFM_ASSERT(read_data == Si7021_TARGET_U1);

}

/***************************************************************************//**
 * @brief
 * 	This function creates instances for the various I2C structs
 *
 * @details
 * This function more specifically passes in the necessary parameters to communicate
 * with the SI7021 via the I2C bus protocol, then calls the i2c_open() function to begin
 * communication
 *
 * @note
 * A similar function would be written for any other arbitrary sensor
 *
 ******************************************************************************/
void si7021_i2c_open(){
	I2C_IO_STRUCT i2c_values;

	i2c_values.scl_pin = SI7021_SCL_PIN;
	i2c_values.scl_port = SI7021_SCL_PORT;
	i2c_values.sda_pin = SI7021_SDA_PIN;
	i2c_values.sda_port = SI7021_SDA_PORT;

	I2C_OPEN_STRUCT i2c_setup;

	i2c_setup.clhr = Si7021_I2C_CLK_RATIO;
	i2c_setup.enable = true;
	i2c_setup.freq = Si7021_I2C_FREQ;
	i2c_setup.master = true;
	i2c_setup.refFreq = Si7021_REF_FREQ;
	i2c_setup.scl_enable = Si7021_SCL_EN;
	i2c_setup.scl_route = Si7021_SCL_LOC;
	i2c_setup.sda_enable = Si7021_SDA_EN;
	i2c_setup.sda_route = Si7021_SDA_LOC;

	i2c_open(Si7021_I2C, &i2c_setup, &i2c_values);

}

/***************************************************************************//**
 * @brief
 * 	This method writes to a target register in the si7021
 *
 * @details
 * 	Using the argument write_command, this function calls i2c_start() to write to the target register,
 * 	communicating that its a write, target register, the number of bytes, and a pointer to where
 * 	the data will be stored for the application.
 *
 * @note
 * 	This write function will look similar for other sensors being communicated to through I2C
 *
 * @param[in] write_command
 * 	desired period for PWM
 *
 *
 * @param[in] bytes
 * 	desired active period for the PWM
 ******************************************************************************/
void si7021_i2c_write(uint32_t write_command, uint8_t bytes){
	i2c_start(Si7021_I2C, Si7021_dev_addr, write_command, &write_data, bytes, false, NO_EVENT);
}

/***************************************************************************//**
 * @brief
 * 	This function reads a target register from the Si7021
 *
 * @details
 * 	Using the argument read_command, this function calls i2c_start() to write to the target register,
 * 	communicating that its a read, target register, the number of bytes, and a pointer to where
 * 	the data will be stored for the application.
 *
 *
 * @note
 *	A similar function would be written for any other arbitrary sensor
 *
 *
 ******************************************************************************/
void si7021_i2c_read(uint32_t read_command, uint8_t bytes, uint32_t event){
	i2c_start(Si7021_I2C, Si7021_dev_addr, read_command, &read_data, bytes, true, event);
}

/***************************************************************************//**
 * @brief
 * 	This function converts the temperature datq into fahrenheit
 *
 * @details
 * Using a formula out of the SI7021 data sheet, I first convert the data into Celcius
 * then convert the celcius temperature into Fahrenheit. The temperature in fahrenheit is returned
 *
 * @note
 *	This function is used to control the LEDs in the scheduler
 *
 * @return
 * 	 Returns temperature in type float
 ******************************************************************************/
float si7021_i2c_temp_convert(){
	float converted_data;
	// Converting to celcius
	converted_data = ((float)read_data * Si7021_TEMP_CONVR) - Si7021_TEMP_SUB;
	// Converting Celcius to Fahrenheit
	converted_data = ((converted_data * 9) / 5) + 32;

	return converted_data;

}

/***************************************************************************//**
 * @brief
 * 	This function converts the relative humidity data into a percentage
 *
 * @details
 * 	Using a formula out of the SI7021 data sheet, I convert the data into a percentage,
 * 	then return the conversion
 *
 * @note
 * 	Allows display of humidity readings
 *
 * @return
 * 	 Returns relative humidity in type float
 ******************************************************************************/
float si7021_i2c_rh_convert(){
	float converted_data;
	// Converting to celcius
	converted_data = ((float)read_data * Si7021_RH_CONVR) - Si7021_RH_SUB;

	return converted_data;

}





