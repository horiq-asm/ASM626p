#include <stdio.h>
#include "pico/stdlib.h"
unsigned int TIMER1,TIMER2,TIMER3,TIMER4,TIMER5,TIMER6,TIMER7,TIMER8,TIMER9,TIMER10,t4sec,t5sec,t6sec;
bool callback_timer0(struct repeating_timer *t)
{
	/* TODO. Add user defined interrupt service routine */

    if( TIMER1)
	   --TIMER1;
    if( TIMER2)
	   --TIMER2;
    if( TIMER3)
	   --TIMER3;
	if( TIMER4){
        if(--t4sec == 0){
	         t4sec=500;
	         --TIMER4;
	    }
	}
    if( TIMER5){
        if(--t5sec == 0){
	        t5sec=500;
	        --TIMER5;
	    }
	}
    if( TIMER6){
        if(--t6sec == 0){
		    t6sec=500;
	        --TIMER6;
	    }
	}
    if( TIMER7)
	   --TIMER7;
	if( TIMER8)
	   --TIMER8;
    if( TIMER9)
	   --TIMER9;
    if( TIMER10);
	   --TIMER10;

}


void CMT0_initialize(void)

{
    static repeating_timer_t timer;
	t4sec=t5sec=t6sec=500;
    add_repeating_timer_ms(1, callback_timer0, NULL, &timer);

}