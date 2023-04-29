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

#define bufferLength 96

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

    dmaConfig();

    maxim_max30102_reset();
    __delay_cycles(100);


    maxim_max30102_init();

   // uint8_t readPointer = getReadPointer();

    //uint8_t writePointer = getWritePointer();

    check();


    sense.head = 0;
    sense.tail = 0;

    P1IE  |= BIT4;
    __bis_SR_register(LPM0_bits|GIE);

    while(!done){
        check();
        while(available() && zz < bufferLength) {
                   redBuffer[zz] = getFIFORed();
                   irBuffer[zz] = getFIFOIR();
                   nextSample();
                   zz++;
                   if(zz == bufferLength)break;
               }

           done2 = 0;
           P1IE  |= BIT4;
           maxim_enable_interrupt();
        if(zz >= bufferLength) {
            done = 1;
            done2 = 1;
            __bic_SR_register(GIE);
        } else {

            __bis_SR_register(LPM0_bits|GIE);
        }


        while(!done2);

    }
    maxim_max30102_shutdown();
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &sys, &dia, onlyheartrate, diasyscalc_en);
    P1OUT |= BIT2;
    __bis_SR_register(LPM4_bits);

    while (1)
    {
        for (i = 0; i < bufferLength; i++)
        {
            while (available() == false) // do we have new data?
                check();                 // Check the sensor for new data

            redBuffer[i] = getFIFORed();
            irBuffer[i] = getFIFOIR();
            nextSample();
        }
        //        uint32_t irBuffer2[100] = {
        //                               226743,
        //                               226709,
        //                               226528,
        //                               226583,
        //                               226514,
        //                               226514,
        //                               226534,
        //                               226576,
        //                               226581,
        //                               226627,
        //                               226606,
        //                               226583,
        //                               226640,
        //                               226717,
        //                               226591,
        //                               226590,
        //                               226677,
        //                               226754,
        //                               226649,
        //                               226616,
        //                               226724,
        //                               226670,
        //                               226457,
        //                               226395,
        //                               226457,
        //                               226537,
        //                               226481,
        //                               226452,
        //                               226555,
        //                               226532,
        //                               226515,
        //                               226471,
        //                               226545,
        //                               226580,
        //                               226468,
        //                               226501,
        //                               226533,
        //                               226498,
        //                               226531,
        //                               226547,
        //                               226503,
        //                               226398,
        //                               226357,
        //                               226352,
        //                               226349,
        //                               226315,
        //                               226440,
        //                               226453,
        //                               226397,
        //                               226374,
        //                               226462,
        //                               226417,
        //                               226358,
        //                               226434,
        //                               226470,
        //                               226409,
        //                               226450,
        //                               226480,
        //                               226420,
        //                               226429,
        //                               226516,
        //                               226284,
        //                               226324,
        //                               226366,
        //                               226313,
        //                               226328,
        //                               226471,
        //                               226459,
        //                               226379,
        //                               226447,
        //                               226359,
        //                               226280,
        //                               226421,
        //                               226332,
        //                               226451,
        //                               226538,
        //                               226500,
        //                               226504,
        //                               226612,
        //                               226523,
        //                               226467,
        //                               226495,
        //                               226441,
        //                               226450,
        //                               226511,
        //                               226498,
        //                               226590,
        //                               226597,
        //                               226591,
        //                               226684,
        //                               226764,
        //                               226754,
        //                               226739,
        //                               226812,
        //                               226663,
        //                               226653,
        //                               226727,
        //                               226529,
        //                               226652,
        //                               226797
        //        };

        //    maxim_max30102_shutdown();
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &sys, &dia, onlyheartrate, diasyscalc_en);
        sprintf(string, "beatAvg=%ld spo2Avg=%f ", heartRate, spo2);
        uca0WriteString(string);
    }

    P1OUT = BIT0;

//    __bis_SR_register(LPM3_bits | GIE);

    PMMCTL0_H = PMMPW_H;    // open PMM
    PMMCTL0_L |= PMMREGOFF; // set Flag to enter LPM4.5 with LPM4 request
    __bis_SR_register(LPM4_bits | GIE);
    __no_operation();

    while (1)
    {
        update = 0;

        for (i = 25; i < bufferLength; i++)
        {
            update = 0;

            for (i = 25; i < bufferLength; i++)
            {
                redBuffer[i - 25] = redBuffer[i];
                irBuffer[i - 25] = irBuffer[i];
            }

            for (i = 76; i < bufferLength; i++)
            {
                while (available() == false) // do we have new data?
                    check();                 // Check the sensor for new data

                redBuffer[i] = getFIFOIR();
                irBuffer[i] = getFIFORed();

                nextSample();
                //                     if (checkForBeat(irValue) == true)
                //                            {
                //                         if(lastBeat == -1) {
                //                             lastBeat = millis();
                //
                //                         } else{
                //                                // We sensed a beat!
                //                                long delta = millis() - lastBeat;
                //                                lastBeat = millis();
                //
                //                                beatsPerMinute = 60 / (delta / 1000.0);
                //
                //                                if (beatsPerMinute < 255 && beatsPerMinute > 20)
                //                                {
                //                                    heartRateAvgArr[heartRateSpot++] = (uint8_t)heartRate; // Store this reading in the array
                //                                    heartRateSpot %= RATE_SIZE;                    // Wrap variable
                //
                //                                    // Take average of readings
                //                                    heartRateAvg = 0;
                //                                    for (uint8_t x = 0; x < RATE_SIZE; x++)
                //                                        heartRateAvg += heartRateAvgArr[x];
                //                                    heartRateAvg /= RATE_SIZE;
                //                                }
                //                            }
                //                            }
            }

            //           maxim_max30102_shutdown();

            lastBeat = -1;
            irValue = irBuffer[99];
            redValue = redBuffer[99];

            maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &sys, &dia, onlyheartrate, diasyscalc_en);

            float diff = (heartRate - heartRateAvg) / float(heartRate);
            diff = diff < 0 ? -diff : diff;

            if (validHeartRate)
            {
                update = 1;
                heartRateAvgArr[heartRateSpot++] = heartRate;
                heartRateSpot %= RATE_SIZE;
                int i;
                heartRateAvg = 0;
                int avgct = 0;
                for (i = 0; i < RATE_SIZE; i++)
                {
                    if (heartRateAvgArr[i] > 0)
                    {
                        avgct++;
                        heartRateAvg += heartRateAvgArr[i];
                    }
                }
                heartRateAvg = heartRateAvg / avgct;
            }

            diff = (spo2 - spo2Avg) / float(spo2);
            diff = diff < 0 ? -diff : diff;
            if (validSPO2)
            {
                update = 1;
                spo2RateAvgArr[spo2Spot++] = spo2;
                spo2Spot %= RATE_SIZE;
                int i;
                spo2Avg = 0;
                int avgct = 0;
                for (i = 0; i < RATE_SIZE; i++)
                {
                    if (spo2RateAvgArr[i] > 0)
                    {
                        spo2Avg += spo2RateAvgArr[i];
                        avgct++;
                    }
                }
                spo2Avg = spo2Avg / avgct;
            }

            if (1)
            {
                //    sprintf(string, "\rIR= %lu Red=%lu beatAvg=%lu spo2Avg=%lu update=%d sys=%lu, dia=%lu", irValue, redValue, heartRate, spo2, update, sys, dia);
                sprintf(string, "beatAvg=%ld spo2Avg=%ld ", heartRate, spo2);
                uca0WriteString(string);
            }
            if (irValue < 50000)
            {
                uca0WriteString("\rNo finger?");
                // resetHeartRate();
            }
        }
    }
}

void testuart()
{
    while (1)
    {
        uca0WriteString("Hello from MSP430FR5994");
        delay(100);
    }
}

void testmillis()
{
    long delta = 0;

    while (1)
    {
        if (millis() - last > 1000)
        {
            last = millis();
            P1OUT ^= BIT4;
        }
    }
}

//
void testdelay()
{
    while (1)
    {
        delay(250);
        P1OUT ^= BIT0;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {

//P1OUT = BIT0;
P1IE  &= ~(BIT4);
P1IFG &= ~BIT4;

done2 = 1;

__bic_SR_register_on_exit(LPM0_bits|GIE);
}
