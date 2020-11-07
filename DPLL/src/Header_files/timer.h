#ifndef TIMER_HG
#define TIMER_HG
//***********************************************************************************
// Include files
//***********************************************************************************
#include "em_timer.h"
#include "scheduler.h"
#include "sleep_routines.h"

//***********************************************************************************
// defined files
//***********************************************************************************
#define TIMER_HZ		1000			// Utilizing ULFRCO oscillator for LETIMERs
#define TIMER_EM		EM2				// Using the ULFRCO, block from entering Energy Mode 4

//***********************************************************************************
// global variables
//***********************************************************************************
typedef struct {
	bool 			debugRun;			// True = keep LETIMER running will halted
	bool 			enable;				// enable the LETIMER upon completion of open
	uint8_t			out_pin_route0;		// out 0 route to gpio port/pin
	uint8_t			out_pin_route1;		// out 1 route to gpio port/pin
	bool			out_pin_0_en;		// enable out 0 route
	bool			out_pin_1_en;		// enable out 1 route
	float			period;				// seconds
	float			active_period;		// seconds
	bool 			comp0_irq_enable;	// enable interrupt on comp0 interrupt
	uint32_t 		comp0_evt;
	bool			comp1_irq_enable;	// enable interrupt on comp1 interrupt
	uint32_t 		comp1_evt;
	bool			uf_irq_enable;		// enable interrupt on uf interrupt
	uint32_t		uf_evt;
}APP_LETIMER_PWM_TypeDef;


//***********************************************************************************
// function prototypes
//***********************************************************************************
void timer_pwm_open(TIMER_TypeDef *timer, APP_LETIMER_PWM_TypeDef *app_letimer_struct);
void timer_start(TIMER_TypeDef *timer, bool enable);
void TIMER0_IRQHandler(void);

#endif
