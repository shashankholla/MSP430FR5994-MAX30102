#include <msp430fr5994.h>
#include <stdio.h>
//
#include "uart.h"
//
char* uca0Receiver;
bool uca0DataReady = false;
//
void uca0Init(void){


    P2SEL0 &= ~(BIT0 | BIT1);
       P2SEL1 |= BIT0 | BIT1;                  // USCI_A0 UART operation

       CSCTL0_H = CSKEY_H;                     // Unlock CS registers
           CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
           CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
           CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
           CSCTL0_H = 0;                           // Lock CS registers

           // Configure USCI_A0 for UART mode
           UCA0CTLW0 = UCSWRST;                    // Put eUSCI in reset
           UCA0CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK
           // Baud Rate calculation
           // 8000000/(16*9600) = 52.083
           // Fractional portion = 0.083
           // User's Guide Table 21-4: UCBRSx = 0x04
           // UCBRFx = int ( (52.083-52)*16) = 1
           UCA0BRW = 52;                           // 8000000/16/9600
           UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
           UCA0CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
           UCA0IE |= UCRXIE;                       // Enable USCI_A0 RX interrupt
}

void uca0WriteByte(const char byte){

    //Enquantooperifricoestiverocupado,espera
    while(!(UCA0IFG&UCTXIFG));
    UCA0TXBUF=byte;
}

void uca0WriteString(char*str){

    do{
        uca0WriteByte(*str);
    }while(*++str);
}

char uca0ReadByte(void){


    unsigned int i=50000;

    do{

        if(UCA0IFG&0x01){
            return UCA0RXBUF;
        }
    }while(--i);

    //Casoitenhachegadoa0antesdealgumbytechegareligarainterrupodaserial,retorna\0comoterminadordastringrecebida
    return'\0';
}

char* uca0ReadString(void){

    static char buffer[RX_BUFF];
    char*temp;
    temp=buffer;

    //Aonotarquearotinasempreacabavaperdendooprimeirobyte,aprimeiracapturafeita
    *temp=UCA0RXBUF;
    temp++;

    //Enquantoreceberbytes!=doterminador'\0',armazenanobufferepassapraprximaposio
    while((*temp=uca0ReadByte())!='\0'){
        ++temp;
    }

    //Retornaumponteiroparaoarraycomastring
    return buffer;
}

#pragma vector = USCI_A0_VECTOR
__interrupt void uartA0(void){

    switch(UCA0IV)
    {
        case 2:
            //Loquechegounobufferderecepo
            uca0Receiver=uca0ReadString();

            //Indicaquearecepojfoilida
            uca0DataReady=true;
            break;
        case 4:

            break;
    }
}
