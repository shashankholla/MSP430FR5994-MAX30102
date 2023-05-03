#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include "uart.h"
#include "config.h"
//#include "heartRate.h"
#include "data.h"
#include "algorithm.h"


#define LENGTH 4000

volatile int adcReading = 0;

const char RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
char rates[RATE_SIZE];    // Array of heart rates
char rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

void testdelay();
char string[40];


//#define __nv    __attribute__((section(".persistent")))
//
//#if defined(__TI_COMPILER_VERSION__)
//#pragma PERSISTENT(readings)
//uint16_t readings[LENGTH] = {0};
//#elif defined(__IAR_SYSTEMS_ICC__)
//__persistent uint16_t readings[LENGTH] = {0};
//#elif defined(__GNUC__)
//uint16_t __attribute__((persistent)) readings[LENGTH] = {0};
//#else
//#error Compiler not supported!
//#endif
#pragma PERSISTENT
float readings[LENGTH] = {0};

#pragma PERSISTENT
float filtered[LENGTH] = {0};
//uint16_t time[LENGTH];
int i = 0, k;
int32_t an_ir_valley_locs[15] ;
float n_peak_interval_sum;
void filter(float *b, int b_length, float *x, int x_length, float* filter) {
      filter[0] = b[0]*x[0];

      for (int i = 1; i < x_length; i++) {
        filter[i] = 0.0;
        for (int j = 0; j <= i; j++) {
          int k = i-j;
          if (j > 0) {
            if ((k < b_length) && (j < x_length)) {
              filter[i] += b[k]*x[j];
            }
            if ((k < x_length)) {
              filter[i] -= 1*filter[k];
            }
          } else {
            if ((k < b_length) && (j < x_length)) {
              filter[i] += (b[k]*x[j]);
            }
          }
        }
      }

//      for(int i=1; i<x_length; i++) filter[i] = filter[i]+filter[i-1];
}

int main(void)
{


    configClock();
    disableUnwantedGPIO();
    initTMR();

    FRCTL0 = FRCTLPW;
    GCCTL0 = FRPWR | 0x02;



          uca0Init();
          uca0WriteString("Hello\n");

    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;        // Sampling time, S&H=16, ADC12 on
    ADC12CTL1 = ADC12SHP;                     // Use sampling timer
    ADC12CTL2 |= ADC12RES_2;                  // 12-bit conversion results
    ADC12MCTL0 |= ADC12VRSEL_0 | ADC12INCH_2; // A1 ADC input select; Vref=AVCC
    ADC12IER0 |= ADC12IE0;                    // Enable ADC conv complete interrupt
    P1OUT &= ~(BIT4);
    while (1)
    {


        if(i == LENGTH) {
            __no_operation();

            int pn_heart_rate, pch_hr_valid;
            for(k=0; k< LENGTH-4; k++){
                readings[k]=( readings[k]+readings[k+1]+ readings[k+2]+ readings[k+3])/(int)4;
            }
            uca0WriteString("4k col\n");
            filter(b, 49, readings, 4000, filtered);
            uca0WriteString("4k filt\n");
            int32_t n_th1=0;

            n_th1 = 10;
            for ( k=0 ; k<15;k++) an_ir_valley_locs[k]=0;
            int32_t n_npks;
            // since we flipped signal, we use peak detector as valley detector
            maxim_find_peaks( an_ir_valley_locs, &n_npks, filtered, LENGTH, n_th1, 100, 15 );//peak_height, peak_distance, max_num_peaks
            n_peak_interval_sum =0;
            if (n_npks>=2){
            for (k=1; k<n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] -an_ir_valley_locs[k -1] ) ;
            n_peak_interval_sum =n_peak_interval_sum/(n_npks-1);
            pn_heart_rate =( (float)60/ (n_peak_interval_sum/1600) );
            pch_hr_valid  = 1;
            }
            else  {
            pn_heart_rate = -999; // unable to calculate because # of peaks are too small
            pch_hr_valid  = 0;
            }

            sprintf(string, "Heart rate: %d\n", pn_heart_rate);
            uca0WriteString(string);
            P1OUT |= (BIT4);
//             __bis_SR_register(LPM0_bits | GIE);

            i = 0;
        }

        __delay_cycles(5000);

        ADC12CTL0 |= ADC12ENC | ADC12SC; // Start sampling/conversion


        __bis_SR_register(LPM0_bits | GIE); // LPM0, ADC12_ISR will force exit

        
        // __no_operation();                   // For debugger


        // if (checkForBeat(adcReading))
        // {
        //     // We sensed a beat!
        //     long delta = millis() - lastBeat;
        //     lastBeat = millis();

        //     beatsPerMinute = 60 / (delta / 1000.0);

        //     if (1)
        //     {
        //         rates[rateSpot++] = (char)beatsPerMinute; // Store this reading in the array
        //         rateSpot %= RATE_SIZE;                    // Wrap variable

        //         // Take average of readings
        //         beatAvg = 0;
        //         for (char x = 0; x < RATE_SIZE; x++)
        //             beatAvg += rates[x];
        //         beatAvg /= RATE_SIZE;
        //     }
        // }

        // sprintf(string, "%ld, %d, %d", millis(), adcReading, beatAvg);
        //                 uca0WriteString(string);
        P1OUT &= ~(BIT4);
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_B_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(ADC12_B_VECTOR))) ADC12_ISR(void)
#else
#error Compiler not supported!
#endif
{
    switch (__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
    case ADC12IV__NONE:
        break; // Vector  0:  No interrupt
    case ADC12IV__ADC12OVIFG:
        break; // Vector  2:  ADC12MEMx Overflow
    case ADC12IV__ADC12TOVIFG:
        break; // Vector  4:  Conversion time overflow
    case ADC12IV__ADC12HIIFG:
        break; // Vector  6:  ADC12BHI
    case ADC12IV__ADC12LOIFG:
        break; // Vector  8:  ADC12BLO
    case ADC12IV__ADC12INIFG:
        break;               // Vector 10:  ADC12BIN
    case ADC12IV__ADC12IFG0: // Vector 12:  ADC12MEM0 Interrupt
        adcReading = ADC12MEM0;
        readings[i] = (float) adcReading;
//        time[i] = millis();
        i++;
        __bic_SR_register_on_exit(LPM0_bits);
        break;
    case ADC12IV__ADC12IFG1:
        break; // Vector 14:  ADC12MEM1
    case ADC12IV__ADC12IFG2:
        break; // Vector 16:  ADC12MEM2
    case ADC12IV__ADC12IFG3:
        break; // Vector 18:  ADC12MEM3
    case ADC12IV__ADC12IFG4:
        break; // Vector 20:  ADC12MEM4
    case ADC12IV__ADC12IFG5:
        break; // Vector 22:  ADC12MEM5
    case ADC12IV__ADC12IFG6:
        break; // Vector 24:  ADC12MEM6
    case ADC12IV__ADC12IFG7:
        break; // Vector 26:  ADC12MEM7
    case ADC12IV__ADC12IFG8:
        break; // Vector 28:  ADC12MEM8
    case ADC12IV__ADC12IFG9:
        break; // Vector 30:  ADC12MEM9
    case ADC12IV__ADC12IFG10:
        break; // Vector 32:  ADC12MEM10
    case ADC12IV__ADC12IFG11:
        break; // Vector 34:  ADC12MEM11
    case ADC12IV__ADC12IFG12:
        break; // Vector 36:  ADC12MEM12
    case ADC12IV__ADC12IFG13:
        break; // Vector 38:  ADC12MEM13
    case ADC12IV__ADC12IFG14:
        break; // Vector 40:  ADC12MEM14
    case ADC12IV__ADC12IFG15:
        break; // Vector 42:  ADC12MEM15
    case ADC12IV__ADC12IFG16:
        break; // Vector 44:  ADC12MEM16
    case ADC12IV__ADC12IFG17:
        break; // Vector 46:  ADC12MEM17
    case ADC12IV__ADC12IFG18:
        break; // Vector 48:  ADC12MEM18
    case ADC12IV__ADC12IFG19:
        break; // Vector 50:  ADC12MEM19
    case ADC12IV__ADC12IFG20:
        break; // Vector 52:  ADC12MEM20
    case ADC12IV__ADC12IFG21:
        break; // Vector 54:  ADC12MEM21
    case ADC12IV__ADC12IFG22:
        break; // Vector 56:  ADC12MEM22
    case ADC12IV__ADC12IFG23:
        break; // Vector 58:  ADC12MEM23
    case ADC12IV__ADC12IFG24:
        break; // Vector 60:  ADC12MEM24
    case ADC12IV__ADC12IFG25:
        break; // Vector 62:  ADC12MEM25
    case ADC12IV__ADC12IFG26:
        break; // Vector 64:  ADC12MEM26
    case ADC12IV__ADC12IFG27:
        break; // Vector 66:  ADC12MEM27
    case ADC12IV__ADC12IFG28:
        break; // Vector 68:  ADC12MEM28
    case ADC12IV__ADC12IFG29:
        break; // Vector 70:  ADC12MEM29
    case ADC12IV__ADC12IFG30:
        break; // Vector 72:  ADC12MEM30
    case ADC12IV__ADC12IFG31:
        break; // Vector 74:  ADC12MEM31
    case ADC12IV__ADC12RDYIFG:
        break; // Vector 76:  ADC12RDY
    default:
        break;
    }
}
