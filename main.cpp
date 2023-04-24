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
#define BIT_SET(x,y) x|=(y)
#define BIT_CLEAR(x,y) x&= ~(y)
char string[100];
long last = 0;

void testdelay();

#define bufferLength 100

uint32_t irBuffer[100];  // infrared LED sensor data
uint32_t redBuffer[100]; // red LED sensor data

int32_t spo2;               // SPO2 value
int8_t validSPO2;         // indicator to show if the SPO2 calculation is valid
int32_t heartRate;         // heart rate value
int8_t validHeartRate;           // indicator to show if the heart rate calculation is valid
bool onlyheartrate = 0;
bool diasyscalc_en = 1;
int32_t sys, dia;
#define RATE_SIZE 8 //Increase this for more averaging. 4 is good.
uint8_t heartRateAvgArr[RATE_SIZE]; //Array of heart rates
uint8_t heartRateSpot = 0;

uint8_t spo2RateAvgArr[RATE_SIZE];
uint8_t spo2Spot =0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int heartRateAvg =90, spo2Avg=95;
int update = 0;
uint32_t irValue, redValue;
unsigned int i;

int main(void) {

    configClock();


       P1OUT = 0;
       P1DIR = 0xFF;

       P1OUT &= ~(BIT0);  // Set SCL and SDA pins to output low
       P1DIR |= (BIT0);   // Set SCL and SDA pins to output mode

       P1OUT = BIT0;
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

       PAOUT = 0;
       PADIR = 0xFFFF;

       PBOUT = 0;
       PBDIR = 0xFFFF;

       PCOUT = 0;
       PCDIR = 0xFFFF;

       PDOUT = 0;
       PDDIR = 0xFFFF;

       PEOUT = 0;
       PEDIR = 0xFFFF;


    initTMR();

//    uca0Init();

//    uca0WriteString("Hello!\n");
    initI2C();


    _enable_interrupts();
    __bis_SR_register(GIE);


    maxim_max30102_reset();
    maxim_max30102_init();



    while(0) {

        if(available()) {
        long irValue = getFIFORed();
        long redValue = getFIFOIR();
        nextSample();
        long time = millis();
        //int16_t filtered;
        //checkForBeat(irValue, &filtered);
        sprintf(string, "%ld, %ld, %ld", time, irValue, redValue);
        uca0WriteString(string);
        } else {
            check();
        }

    }
    for (i = 0; i < 40; i++)
                   {
        while (available() == false) //do we have new data?
              check(); //Check the sensor for new data

                       redBuffer[i] = getFIFOIR();
                        irBuffer[i] = getFIFORed();
                        nextSample();
                   }

    maxim_max30102_shutdown();
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &sys, &dia, onlyheartrate, diasyscalc_en);



    P1OUT = BIT0;


//    __bis_SR_register( LPM3_bits | GIE );

    PMMCTL0_H = PMMPW_H; // open PMM
    PMMCTL0_L |= PMMREGOFF; // set Flag to enter LPM4.5 with LPM4 request
    __bis_SR_register(LPM4_bits|GIE);
    __no_operation();

    while (1)
        {
            update = 0;

            for (i = 25; i < bufferLength; i++)
                {
                  redBuffer[i - 25] = redBuffer[i];
                  irBuffer[i - 25] = irBuffer[i];
                }

            for (i = 76; i < bufferLength; i++)
                {
                while (available() == false) //do we have new data?
                              check(); //Check the sensor for new data

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



           float diff = (heartRate - heartRateAvg)/float(heartRate);
           diff = diff < 0 ? -diff : diff;

           if(validHeartRate) {
               update = 1;
               heartRateAvgArr[heartRateSpot++] = heartRate;
               heartRateSpot %= RATE_SIZE;
               int i;
               heartRateAvg = 0;
               int avgct = 0;
               for(i=0; i < RATE_SIZE; i++) {
                   if(heartRateAvgArr[i] > 0){
                       avgct++;
                       heartRateAvg += heartRateAvgArr[i];
                   }
               }
               heartRateAvg = heartRateAvg/avgct;
           }

           diff = (spo2 - spo2Avg)/float(spo2);
           diff = diff < 0 ? -diff : diff;
           if(validSPO2) {
               update=1;
                          spo2RateAvgArr[spo2Spot++] = spo2;
                          spo2Spot %= RATE_SIZE;
                          int i;
                          spo2Avg = 0;
                          int avgct = 0;
                          for(i=0; i < RATE_SIZE; i++) {
                              if(spo2RateAvgArr[i] > 0){
                              spo2Avg += spo2RateAvgArr[i];
                              avgct++;
                              }
                          }
                          spo2Avg = spo2Avg/avgct;
                      }

           if(1) {
//            last = millis();
               sprintf(string, "\rIR= %lu Red=%lu beatAvg=%lu spo2Avg=%lu update=%d sys=%lu, dia=%lu", irValue, redValue, heartRate, spo2, update, sys, dia);
           // sprintf(string, "beatAvg=%ld spo2Avg=%ld ", heartRate, spo2);
            uca0WriteString(string);
            }
//           __bis_SR_register( LPM3_bits | GIE );
//           break;
           if (irValue < 50000){
               uca0WriteString("\rNo finger?");
              // resetHeartRate();
           }



        }


}

void testuart() {
    while(1) {
    uca0WriteString("Hello from MSP430FR5994");
    delay(100);
    }
}

//
//void testmillis() {
//    long delta = 0;
//
//
//    while(1) {
//           if(millis() - last > 1000) {
//               last = millis();
//               P1OUT ^= BIT4;
//           }
//        }
//}

//
void testdelay() {
    while(1) {
           delay(250);
               P1OUT ^= BIT0;
        }
}
