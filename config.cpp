#include <msp430.h>
#include "config.h"
//#include "driverlib.h"
#define BIT_SET(x,y) x|=(y)
#define BIT_CLEAR(x,y) x&= ~(y)

static volatile uint32_t TIMER_MS_COUNT = 0;

void disableUnwantedGPIO(void) {
    
    P1OUT = 0;
    P1DIR = 0xFF;

    P1OUT &= ~(BIT2);
    P1DIR |= (BIT2);

    P1DIR &= ~(BIT4);
    P1OUT |= BIT4;
    P1REN |= BIT4;
    P1IE  |= BIT4;
    P1IES |= BIT4;
    P1IFG &= ~BIT4;  
    
    
    P2OUT = 0;
    P2DIR = 0xFF;

    P3OUT = 0;
    P3DIR = 0xFF;

    P4OUT = 0;
    P4DIR = 0xFF;

    P5OUT = 0;
    P5DIR = 0xFF;

    P6OUT = 0;
    P6DIR = 0xFF;

    P8OUT = 0;
    P8DIR = 0xFF;

    P9OUT = 0;
    P9DIR = 0xFF;

//    PAOUT = 0;
//    PADIR = 0xFFFF;

    PBOUT = 0;
    PBDIR = 0xFFFF;

    PCOUT = 0;
    PCDIR = 0xFFFF;

    PDOUT = 0;
    PDDIR = 0xFFFF;

    PEOUT = 0;
    PEDIR = 0xFFFF;
}

void dmaConfig(void) {

    DMA4CTL = 0;                                // disable DMA channel 0
    DMACTL4 = DMA4TSEL_0;                      // select UCB0RXIFG0 dma trigger
    DMA4CTL |= DMADSTINCR_3;                    // incremet dest. addr after dma
    DMA4CTL |= (DMADSTBYTE + DMASRCBYTE);       // byte to byte transfer
    DMA4SA = (__SFR_FARPTR) (unsigned long) &UCB2RXBUF;        // set DMA dest address register
}


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
                //BIT_SET(TA3CCTL0, CCIE);
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


