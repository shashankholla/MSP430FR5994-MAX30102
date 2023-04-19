#include <msp430.h>
#include "config.h"
//#include "driverlib.h"
#define BIT_SET(x,y) x|=(y)
#define BIT_CLEAR(x,y) x&= ~(y)

static volatile uint32_t TIMER_MS_COUNT = 0;

void configClock (void)
{
        WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
        PM5CTL0 &= ~LOCKLPM5;

        P1DIR = BIT0| BIT4; // Set P1.0 and P1.4 for debugging timers

        // Disable the GPIO power-on default high-impedance mode to activate
        // previously configured port settings
        PM5CTL0 &= ~LOCKLPM5;

        //Config clock

        CSCTL0 = CSKEY;

        CSCTL1 = DCORSEL | DCOFSEL_3;//DCO 8MHz
        CSCTL2 |= (SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK);
        CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
        CSCTL4 = 0;

}

void initTMR (void)
{
    //Init Timer0 for delay
            TA0CTL |= (MC__CONTINOUS | TASSEL_2 | ID_3);

            //Init Timer3 for millis()
            TA3CTL = 0;

                BIT_SET(TA3CTL, TACLR);
                // Turn the timer off
                BIT_CLEAR(TA3CTL, MC_3);
                // Set the clock source to SMCLK (1MHz)
                BIT_SET(TA3CTL, TASSEL_2);
                // Set divider to 1
                BIT_SET(TA3CTL, ID_3);
                // Turn of capture mode
                BIT_CLEAR(TA3CCTL0, CM);
                // Enable compare mode
                BIT_CLEAR(TA3CCTL0, CAP);
                // Set target value for the interruption
                // 1_000_000 hz / 1000 = 1000 ticks per ms
                TA3CCR0 = 1000;
                // Enable interrupt when reaching CCR0 value
                BIT_SET(TA3CCTL0, CCIE);
                // Clean interrupt flag, if any
                BIT_CLEAR(TA3CCTL0, CCIFG);
                // Start the timer in UP mode
                BIT_SET(TA3CTL, MC_1);
}

void delay (const unsigned int tmr)
{
    //t = count * Taclk
    //count = t*fACLK
    uint32_t x = 1000;

    while(x--){
    TA0R = 0;
    while(TA0R < tmr);
    }
}


uint32_t millis() {
    uint32_t ms = 0;
    ms = TIMER_MS_COUNT;
    return ms;
}

#pragma vector=TIMER3_A0_VECTOR
__interrupt void millis_isr() {
    // Increment the counter
    TIMER_MS_COUNT++;
}


