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

void testdelay();

#define bufferLength 100

uint32_t irBuffer[100];  // infrared LED sensor data
uint32_t redBuffer[100]; // red LED sensor data

float spo2;          // SPO2 value
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

    uca0Init();

    uca0WriteString("Hello!\n");
    initI2C();

    _enable_interrupts();
    __bis_SR_register(GIE);

    maxim_max30102_reset();
    maxim_max30102_init();

    for (i = 0; i < 100; i++)
    {
        while (available() == false) // do we have new data?
            check();                 // Check the sensor for new data

        redBuffer[i] = getFIFOIR();
        irBuffer[i] = getFIFORed();
        nextSample();
    }

    float ratio, correl;
    rf_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &ratio, &correl);

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
            while (available() == false) // do we have new data?
                check();                 // Check the sensor for new data

            redBuffer[i] = getFIFOIR();
            irBuffer[i] = getFIFORed();

            nextSample();
        }

        irValue = irBuffer[99];
        redValue = redBuffer[99];

        rf_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, &ratio, &correl);

        if (1)
        {
            sprintf(string, "\rIR=%d Red=%d beatAvg=%d spo2Avg=%d", (int)irValue, (int)redValue, (int)heartRate, (int)spo2);
            uca0WriteString(string);
        }
        if (irValue < 50000)
        {
            uca0WriteString("\rNo finger?");
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
