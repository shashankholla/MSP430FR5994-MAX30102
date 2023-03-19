#include <msp430.h>
#include <stdint.h>
#include "i2c.h"

#define LED_OUT     P1OUT
#define LED_DIR     P1DIR
#define LED0_PIN    BIT0
#define LED1_PIN    BIT1

#define SLAVE_ADDR          0x57

void initI2C()
{
    // Configure GPIO
//       LED_OUT &= ~(LED0_PIN | LED1_PIN); // P1 setup for LED & reset output
//       LED_DIR |= (LED0_PIN | LED1_PIN);
//
//       // I2C pins
       P7SEL0 |= BIT0 | BIT1;
       P7SEL1 &= ~(BIT0 | BIT1);
       P7REN |= BIT0 | BIT1;

       // Disable the GPIO power-on default high-impedance mode to activate
       // previously configured port settings
       PM5CTL0 &= ~LOCKLPM5;


    UCB2CTLW0 |= UCSWRST;                     // Enable SW reset
    UCB2CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC; // I2C master mode, SMCLK
    UCB2BRW = 10; //3;//10;                            // fSCL = SMCLK/10 = ~100kHz
    UCB2I2CSA = SLAVE_ADDR;                   // Slave Address
    UCB2CTLW0 &= ~UCSWRST;                  // Clear SW reset, resume operation
    UCB2IE |= UCNACKIE;//enable NACK ISR (TX and RX?)

}

void i2c_start (uint8_t dev_addr,unsigned int RW)
{
    while(UCB2STAT & UCBBUSY);//check if SDA and SCL are idle

    UCB2I2CSA = dev_addr;

    if(RW == READ){UCB2CTLW0 &= ~UCTR;}
    else{UCB2CTLW0 |= UCTR;}

    UCB2CTLW0 |= UCTXSTT;

    while (UCB2CTLW0 & UCTXSTT);//wait till the whole address has been sent and ACK'ed
}

void i2c_stop (void)
{
    UCB2CTLW0 |= UCTXSTP;//stop
    while(UCB2CTLW0 & UCTXSTP);//wait for a stop to happen
}

void i2c_repeated_start(uint8_t dev_addr, unsigned int RW)
{
    UCB2I2CSA = dev_addr;

    if(RW == READ){UCB2CTLW0 &= ~UCTR;}
    else{UCB2CTLW0 |= UCTR;}

    UCB2CTLW0 |= UCTXSTT;

    while (UCB2CTLW0 & UCTXSTT);//wait till the whole address has been sent and ACK'ed
}

void i2c_write_reg(uint8_t slv_addr, uint8_t reg_addr, uint8_t data){

    while(UCB2STAT & UCBBUSY);

    UCB2I2CSA = slv_addr;                   // set slave address
    UCB2CTLW0 |= UCTR | UCTXSTT;            // transmitter mode and START condition.

    while (UCB2CTLW0 & UCTXSTT);
    UCB2TXBUF = reg_addr;
    while(!(UCB2IFG & UCTXIFG0));
    UCB2TXBUF = data;
    while(!(UCB2IFG & UCTXIFG0));

    UCB2CTLW0 |= UCTXSTP;
    while(UCB2CTLW0 & UCTXSTP);             // wait for stop
}

void i2c_write (uint8_t data)
{

    while(!(UCB2IFG & UCTXIFG0));

    UCB2TXBUF = data;
    while(!(UCB2IFG & UCTXIFG0));
    while(UCB2CTLW0 & UCTXIFG0);//1 means data is sent completely

}

void i2c_read (uint8_t * led,unsigned int RxByteCtr)
{
    int i = 0;
    for(i = 0; i < RxByteCtr;i++)
    {
        while(!(UCB2IFG & UCRXIFG0));//make sure rx buffer got data

        if(i == RxByteCtr - 1) {
            while((UCB2CTLW0 & UCTXSTT));
            UCB2CTLW0 |= UCTXSTP;//stop
        }



        led[i] = UCB2RXBUF;
    }

    while(UCB2CTLW0 & UCTXSTP);//wait for a stop to happen
}
