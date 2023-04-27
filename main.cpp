#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include "uart.h"
#include "config.h"
#include "i2c.h"
#include "max30102.h"
#include "algorithm.h"
#include "heartRate.h"

static volatile uint32_t TIMER_MS_COUNT = 0;
volatile uint32_t x;
#define BIT_SET(x, y) x |= (y)
#define BIT_CLEAR(x, y) x &= ~(y)
char string[100];
long last = 0;
volatile int zz = 0;
volatile int done = 0, done2 = 0;

void testdelay();

#define bufferLength 100

uint32_t irBuffer[bufferLength];  // infrared LED sensor data
uint32_t redBuffer[bufferLength]; // red LED sensor data

float spo2;            // SPO2 value
int8_t validSPO2;      // indicator to show if the SPO2 calculation is valid
int32_t heartRate;     // heart rate value
int8_t validHeartRate; // indicator to show if the heart rate calculation is valid
bool onlyheartrate = 0;
bool diasyscalc_en = 1;
int32_t sys, dia;
#define RATE_SIZE 8                 // Increase this for more averaging. 4 is good.
uint8_t heartRateAvgArr[RATE_SIZE]; // Array of heart rates
uint8_t heartRateSpot = 0;

uint8_t spo2RateAvgArr[RATE_SIZE];
uint8_t spo2Spot = 0;
long lastBeat = 0; // Time at which the last beat occurred

float beatsPerMinute;
int heartRateAvg = 90, spo2Avg = 95;
int update = 0;
uint32_t irValue, redValue;
unsigned int i;

int main(void)
{

    configClock();
    disableUnwantedGPIO();

    initTMR();

    //uca0Init();

    //uca0WriteString("Hello!\n");
    initI2C();



    maxim_max30102_reset();
    __delay_cycles(100);

    __bis_SR_register(GIE);
    maxim_max30102_init();
    __bis_SR_register(LPM0_bits);

    while(!done){
        //check();
        check_interrupt_on_full();
        while(available() && zz < bufferLength) {
                   redBuffer[zz] = getFIFORed();
                   irBuffer[zz] = getFIFOIR();
                   nextSample();
                   zz++;
                   if(zz == bufferLength)break;
               }


           maxim_enable_interrupt();
           done2 = 0;

        if(zz == bufferLength) {
            done = 1;
            done2 = 1;
            __bic_SR_register(GIE);
        } else {
            __bis_SR_register(GIE);
        }

        while(!done2);

    }
    maxim_max30102_shutdown();
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &sys, &dia, onlyheartrate, diasyscalc_en);
    P1OUT |= BIT2;
    __bis_SR_register(LPM0_bits);

}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {

//P1OUT = BIT0;
P1IFG &= ~BIT4;
done2 = 1;

__bic_SR_register_on_exit(LPM0_bits|GIE);
}
