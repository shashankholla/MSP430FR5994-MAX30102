/*
        ________________                    ________________
        |               | SCL/P1.7          |               |
        |               |-------------------|               |
        |               |                   |               |
        |   msp430      |                   |    max30102   |
        |               | SDA/P1.6          |               |
        |               |-------------------|               |
        |_______________|                   |_______________|


*/
#include <msp430.h>
#include "max30102.h"
#include "i2c.h"
#include "config.h"
#include "uart.h"
#include <stdio.h>
uint32_t aun_ir_buffer[100];  // infrared LED sensor data
uint32_t aun_red_buffer[100]; // red LED sensor data
int32_t n_ir_buffer_length;   // data length
int32_t n_spo2;               // SPO2 value
int8_t ch_spo2_valid;         // indicator to show if the SPO2 calculation is valid
int32_t n_heart_rate;         // heart rate value
int8_t ch_hr_valid;           // indicator to show if the heart rate calculation is valid
uint8_t uch_dummy;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;

    configClock();
    initI2C();
    initTMR();
    _enable_interrupts();
    uca0Init();

    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |= BIT0 | BIT1; // USCI_A0 UART operation

    __bis_SR_register(GIE);
    uca0WriteString("hello");
    maxim_max30102_reset();

    delay(10); // 10 ms

    maxim_max30102_init();

    while (1)
    {
        long irValue = particleSensor.getIR();

        if (checkForBeat(irValue) == true)
        {
            // We sensed a beat!
            long delta = millis() - lastBeat;
            lastBeat = millis();

            beatsPerMinute = 60 / (delta / 1000.0);

            if (beatsPerMinute < 255 && beatsPerMinute > 20)
            {
                rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
                rateSpot %= RATE_SIZE;                    // Wrap variable

                // Take average of readings
                beatAvg = 0;
                for (byte x = 0; x < RATE_SIZE; x++)
                    beatAvg += rates[x];
                beatAvg /= RATE_SIZE;
            }
        }

        Serial.print("IR=");
        Serial.print(irValue);
        Serial.print(", BPM=");
        Serial.print(beatsPerMinute);
        Serial.print(", Avg BPM=");
        Serial.print(beatAvg);

        if (irValue < 50000)
            Serial.print(" No finger?");
    }

    return 0;
}
