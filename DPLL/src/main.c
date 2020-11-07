/***************************************************************************//**
 * @file
 * @brief Simple LED Blink Demo for SLSTK3402A
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/* System include statements */
#include <stdint.h>
#include <stdbool.h>

/* Silicon Labs include statements */
#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_letimer.h"
#include "bsp.h"

/* The developer's include statements */
#include "main.h"
#include "app.h"


int main(void)
{
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;

  /* Chip errata */
  CHIP_Init();

  /* Init DCDC regulator and HFXO with kit specific parameters */
  /* Init DCDC regulator and HFXO with kit specific parameters */
  /* Initialize DCDC. Always start in low-noise mode. */
  EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);
  em23Init.vScaleEM23Voltage = emuVScaleEM23_LowPower;
  EMU_EM23Init(&em23Init);
  CMU_HFXOInit(&hfxoInit);

  /* Switch HFCLK to HFRCO and disable HFXO */
  CMU_HFRCOBandSet(cmuHFRCOFreq_26M0Hz); // Set main CPU osc. to 26 MHz
  CMU_OscillatorEnable(cmuOsc_HFRCO, true, true);
  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
  CMU_OscillatorEnable(cmuOsc_HFXO, false, false);

  /* Call application program to open / initialize all required peripheral */
  app_peripheral_setup();

  /* Call to start the LETIMER operation */
//  letimer_start(LETIMER0, true);

  EFM_ASSERT(get_scheduled_event() & BOOT_UP_EVT);

  /* Infinite blink loop */
  while (1) {

	  if (!get_scheduled_event()){
		  enter_sleep();
	  }
	  //EMU_EnterEM1();
	  if(get_scheduled_event() & LETIMER0_UF_EVT) {
		  scheduled_letimer0_uf_evt();
	  }
	  if(get_scheduled_event() & LETIMER0_COMP0_EVT){
		  scheduled_letimer0_comp0_evt();
	  }
	  if(get_scheduled_event() & LETIMER0_COMP1_EVT){
		  scheduled_letimer0_comp1_evt();
	  }
	  if(get_scheduled_event() & SI7021_TEMP_DONE_EVT){
		  scheduled_si7021_temp_done_evt();
	  }
	  if(get_scheduled_event() & SI7021_RH_DONE_EVT){
		  scheduled_si7021_rh_done_evt();
	  }
	  if(get_scheduled_event() & BOOT_UP_EVT){
		  scheduled_boot_up_evt();
	  }
	  if(get_scheduled_event() & LEUART0_TX_DONE_EVT){
		  scheduled_leuart0_tx_done_evt();
	  }
  }
}
