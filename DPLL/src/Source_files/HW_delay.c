/**
 * @file HW_delay.c
 * @author Keith Graham
 * @date April 19th, 2020
 * @brief Contains hardware delay function
 *
 */
#ifndef SRC_SOURCE_FILES_HW_DELAY_C_
#define SRC_SOURCE_FILES_HW_DELAY_C_

#include "HW_delay.h"

void timer_delay(uint32_t ms_delay){
	uint32_t timer_clk_freq = CMU_ClockFreqGet(cmuClock_HFPER);
	uint32_t delay_count = ms_delay *(timer_clk_freq/1000) / 1024;
	CMU_ClockEnable(cmuClock_TIMER0, true);
	TIMER_Init_TypeDef delay_counter_init = TIMER_INIT_DEFAULT;
	delay_counter_init.oneShot = true;
	delay_counter_init.enable = false;
	delay_counter_init.mode = timerModeDown;
	delay_counter_init.prescale = timerPrescale1024;
	delay_counter_init.debugRun = false;
	TIMER_Init(TIMER0, &delay_counter_init);
	TIMER0->CNT = delay_count;
	TIMER_Enable(TIMER0, true);
	while (TIMER0->CNT != 00);
	TIMER_Enable(TIMER0, false);
	CMU_ClockEnable(cmuClock_TIMER0, false);
}

#endif /* SRC_SOURCE_FILES_HW_DELAY_C_ */
