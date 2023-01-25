/*
 * button.c
 *
 *  Created on: 26 juin 2019
 *      Author: Nirgal
 */
#include "button.h"
#include "config.h"
#include "stm32f1_gpio.h"
#include "macro_types.h"
#include "systick.h"

#define LONG_PRESS_DURATION	1000	//unit� : [1ms] => 1 seconde.

static void process_ms(void);

static volatile bool_e flag_10ms;
static volatile uint32_t t = 0;
static bool_e initialized = FALSE;

void BUTTON_init(void)
{
	BSP_GPIO_PinCfg(GPIOA,GPIO_PIN_8,GPIO_MODE_INPUT,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH);

	Systick_add_callback_function(&process_ms);

	initialized = TRUE;
}

static void process_ms(void)
{
	static uint32_t t10ms = 0;
	t10ms = (t10ms + 1)%10;		//incr�mentation de la variable t10ms (modulo 10 !)
	if(!t10ms)
		flag_10ms = TRUE; //toutes les 10ms, on l�ve ce flag.
	if(t)
		t--;
}


button_event_e BUTTON_state_machine(void)
{
	typedef enum
	{
		INIT = 0,
		WAIT_BUTTON,
		BUTTON_PRESSED,
		WAIT_RELEASE
	}state_e;

	static state_e state = INIT;

	button_event_e ret = BUTTON_EVENT_NONE;
	bool_e current_button;

	if(flag_10ms && initialized)
	{
		flag_10ms = FALSE;
		current_button = !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_8);
		switch(state)
		{
			case INIT:
				state = WAIT_BUTTON;
				break;
			case WAIT_BUTTON:
				if(current_button)
				{
					t=LONG_PRESS_DURATION;
					state = BUTTON_PRESSED;
				}
				break;
			case BUTTON_PRESSED:
				if(t==0)
				{
					ret = BUTTON_EVENT_LONG_PRESS;
					state = WAIT_RELEASE;
				}
				else if(!current_button)
				{
					ret = BUTTON_EVENT_SHORT_PRESS;
					state = WAIT_BUTTON;
				}
				break;

			case WAIT_RELEASE:
				if(!current_button)
				{
					state = WAIT_BUTTON;
				}
				break;
			default:
				state = INIT;
				break;
		}
	}
	return ret;
}
