#include <msp430.h>
#include <stdint.h>
#include "i2c.h"
#include "max30102.h"
#include "config.h"

#define MODE_CONFIG 0x03
#define ACTIVE_LEDS 2
#define MULTI_LED_CONTROL 0x21

#define SLAVE_ADDR 0x57
// register addresses
#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12
#define REG_TEMP_INTR 0x1F
#define REG_TEMP_FRAC 0x20
#define REG_TEMP_CONFIG 0x21
#define REG_PROX_INT_THRESH 0x30
#define REG_REV_ID 0xFE
#define REG_PART_ID 0xFF

#define I2C_BUFFER_LENGTH 192

volatile bool dmaDone = 0;

static char allData[193];
struct reg_write
{
    uint8_t addr;
    uint8_t data;
};


uint32_t readThreeBytes(int toGet)
{
    uint8_t temp[3]; // Array of 4 bytes that we will convert into long
    i2c_read2(temp, 3, toGet);

    uint32_t tempLong = 0;
    uint32_t un_temp;
    un_temp = temp[0];
    un_temp <<= 16;
    tempLong += un_temp;

    un_temp = temp[1];
    un_temp <<= 8;
    tempLong += un_temp;

    un_temp = temp[2];
    tempLong += un_temp;
    tempLong &= 0x3FFFF;
    return tempLong;
}

uint16_t check(void)
{
    // Read register FIDO_DATA in (3-uint8_t * number of active LED) chunks
    // Until FIFO_RD_PTR = FIFO_WR_PTR

    uint8_t readPointer = getReadPointer();

    uint8_t writePointer = getWritePointer();

    uint16_t toGet;

    int numberOfSamples = 0;
    int activeLEDs = ACTIVE_LEDS;
    // Do we have new data?
    if(readPointer == writePointer){
        writePointer = 32;
        readPointer = 0;
    }
    if (readPointer != writePointer)
    {
        // Calculate the number of readings we need to get from sensor
        numberOfSamples = writePointer - readPointer;
        if (numberOfSamples < 0)
            numberOfSamples += 32; // Wrap condition

        // We now have the number of readings, now calc bytes to read
        // For this example we are just doing Red and IR (3 bytes each)
        int bytesLeftToRead = numberOfSamples * activeLEDs * 3;

        // Get ready to read a burst of data from the FIFO register
        // We may need to read as many as 288 bytes so we read in blocks no larger than I2C_BUFFER_LENGTH
        // I2C_BUFFER_LENGTH changes based on the platform. 64 bytes for SAMD21, 32 bytes for Uno.
        // Wire.requestFrom() is limited to BUFFER_LENGTH which is 32 on the Uno

           toGet = bytesLeftToRead;
            if (toGet > I2C_BUFFER_LENGTH)
            {
                // If toGet is 32 this is bad because we read 6 bytes (Red+IR * 3 = 6) at a time
                // 32 % 6 = 2 left over. We don't want to request 32 bytes, we want to request 30.
                // 32 % 9 (Red+IR+GREEN) = 5 left over. We want to request 27.

                toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % (activeLEDs * 3)); // Trim toGet to be a multiple of the samples we need to read
            }

            bytesLeftToRead -= toGet;

            // Request toGet number of bytes from sensor
            //uint8_t readclear = UCB2RXBUF;

            UCB2CTLW0 |= UCSWRST;
            UCB2CTLW1 = UCASTP_2;
            UCB2CTLW0 &= ~(UCTXSTP);
            UCB2TBCNT = toGet;
            UCB2CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC; // I2C master mode, SMCLK
            UCB2BRW = 19;                                           // 3;//10;                            // fSCL = SMCLK/10 = ~100kHz
            UCB2I2CSA = SLAVE_ADDR;                                 // Slave Address
            UCB2CTLW0 &= ~UCSWRST;

            __bis_SR_register(GIE);
            DMA4CTL &=~DMAEN;
            DMA4CTL |=DMADT_4|DMADSTINCR_3|DMASRCINCR_0|DMASRCBYTE__BYTE|DMADSTBYTE__BYTE|DMALEVEL__EDGE|DMAIE;
            DMACTL2    |= DMA4TSEL__UCB2RXIFG0;
            DMA4SZ     = toGet;
            DMA4DA = (__SFR_FARPTR) (unsigned long) allData;
            DMA4CTL |= DMAEN;

            dmaDone = 0;
            i2c_start(SLAVE_ADDR, WRITE);
            i2c_write(REG_FIFO_DATA);
            i2c_repeated_start(SLAVE_ADDR, READ);


            while(!dmaDone);
            DMA4CTL &=~DMAEN;
            //getAllBytes(toGet, allData);
            uint32_t tempLong = 0;
            uint32_t un_temp;



            for(int i = 0; i < toGet;) {
              un_temp = allData[i];
              un_temp <<= 16;
              tempLong += un_temp;

              un_temp = allData[i+1];
              un_temp <<= 8;
              tempLong += un_temp;

              un_temp = allData[i+2];
              tempLong += un_temp;
              tempLong &= 0x3FFFF;
              sense.red[sense.head] = tempLong;

                tempLong = 0;
                uint32_t un_temp;
                un_temp = allData[i+3];
                un_temp <<= 16;
                tempLong += un_temp;

                un_temp = allData[i+4];
                un_temp <<= 8;
                tempLong += un_temp;

                un_temp = allData[i+5];
                tempLong += un_temp;
                tempLong &= 0x3FFFF;
                sense.IR[sense.head] = tempLong;

               sense.head++;                                  // Advance the head of the storage struct
               sense.head %= STORAGE_SIZE;
                i+=6;

            }
    } // End readPtr != writePtr

    return (toGet); // Let the world know how much new data we found
}

bool safeCheck(uint8_t maxTimeToCheck)
{
    uint32_t markTime = millis();

    while (1)
    {
        if (millis() - markTime > maxTimeToCheck)
            return (false);
        uint16_t x = check();
        if (x > 0) // We found new data!
            return (true);


    }
}

uint32_t getIR()
{
    // Check the sensor for new data for 250ms
    if (safeCheck(10))
        return sense.IR[sense.head];
    else
        return (0); // Sensor failed to find new data
}

uint32_t getRed()
{

    if (safeCheck(10))
           return sense.red[sense.head];
       else
           return (0); // Sensor failed to find new data

}

uint8_t available(void)
{
  int8_t numberOfSamples = sense.head - sense.tail;
  if (numberOfSamples < 0) numberOfSamples += STORAGE_SIZE;

  return (numberOfSamples);
}

void nextSample(void)
{
  if(available()) //Only advance the tail if new data is available
  {
    sense.tail++;
    sense.tail %= STORAGE_SIZE; //Wrap condition
  }
}


uint32_t getFIFORed(void)
{
  return (sense.red[sense.tail]);
}

//Report the next IR value in the FIFO
uint32_t getFIFOIR(void)
{
  return (sense.IR[sense.tail]);
}


uint8_t readRegister8(uint8_t addr, uint8_t reg)
{

    i2c_start(addr, WRITE);
    i2c_write(reg);

    // i2c_start(SLAVE_ADDR,READ);
    i2c_repeated_start(addr, READ);

    uint8_t data[1];
    i2c_read1byte(data);
    return data[0];
}

// Read the FIFO Write Pointer
uint8_t getWritePointer(void)
{
    return (readRegister8(SLAVE_ADDR, REG_FIFO_WR_PTR));
}

// Read the FIFO Read Pointer
uint8_t getReadPointer(void)
{
    return (readRegister8(SLAVE_ADDR, REG_FIFO_RD_PTR));
}

void writeRegister8(uint8_t address, uint8_t reg, uint8_t value)
{
    i2c_start(address, WRITE);
    i2c_write(reg);
    i2c_write(value);
    i2c_stop();
}

void maxim_max30102_reset(void)
{
    //     //S SLA W ACK 0x09 ACK 0x40 ACK P
    //     //shutdown IC for LPM
    //     i2c_start (SLAVE_ADDR,WRITE);
    //     i2c_write(REG_MODE_CONFIG);
    //     i2c_write(0x40);//shut down IC for LPM usage
    //     i2c_stop ();
    volatile uint8_t px = readRegister8(SLAVE_ADDR, 0x00);
    writeRegister8(SLAVE_ADDR, REG_MODE_CONFIG, 0x40); // Reset

}

void maxim_max30102_init(void)
{
  /*  struct reg_write max30102_config[] = {
        {REG_INTR_ENABLE_1, 0x00},
        {REG_INTR_ENABLE_2, 0x00},
        {REG_FIFO_WR_PTR, 0x00},
        {REG_OVF_COUNTER, 0x00},
        {REG_FIFO_RD_PTR, 0x00},
        {REG_FIFO_CONFIG, 0x1F},
        {REG_MODE_CONFIG, 0x03},
        {REG_SPO2_CONFIG, 0x27},
        {REG_LED1_PA, 0x3C}, //was  0x0A
        {REG_LED2_PA, 0x3C},
        {REG_LED2_PA + 1, 0x3C},
        {REG_MULTI_LED_CTRL1, 0x21},
        {REG_MULTI_LED_CTRL2, 0x00},
    };*/

    struct reg_write max30102_config[] = {
        {REG_INTR_ENABLE_1, 0x80},
        {REG_INTR_ENABLE_2, 0x00},
        {REG_FIFO_CONFIG, 0x40}, // was 5F
        {REG_MODE_CONFIG, MODE_CONFIG},
        {REG_SPO2_CONFIG, 0x24},
        {REG_LED1_PA, 0x3C}, //IR led
        {REG_LED2_PA, 0x3C}, //was  0x3C //Red led
        {REG_LED2_PA + 1, 0x3C},
        {REG_LED2_PA + 2, 0x3C},
        {REG_MULTI_LED_CTRL1, MULTI_LED_CONTROL},
        {REG_MULTI_LED_CTRL2, 0x00},
        {REG_FIFO_WR_PTR, 0x00},
        {REG_OVF_COUNTER, 0x00},
        {REG_FIFO_RD_PTR, 0x00},
    };

    for (int i = 0; i < sizeof(max30102_config) / sizeof(max30102_config[0]); i++)
    {
        writeRegister8(SLAVE_ADDR, max30102_config[i].addr, max30102_config[i].data);
    }
}

void maxim_enable_interrupt(void) {
//    writeRegister8(SLAVE_ADDR, 0x00, 0x00);
    volatile uint8_t px = readRegister8(SLAVE_ADDR, 0x00);
    writeRegister8(SLAVE_ADDR, REG_INTR_ENABLE_1, 0x80);
  //  volatile uint8_t px = readRegister8(SLAVE_ADDR, 0x00);
//    volatile uint8_t py = readRegister8(SLAVE_ADDR, 0x02);

}

void maxim_max30102_shutdown(void)
{

    struct reg_write max30102_config[] = {

        {REG_MODE_CONFIG, 0x83},

    };

    for (int i = 0; i < sizeof(max30102_config) / sizeof(max30102_config[0]); i++)
    {
        writeRegister8(SLAVE_ADDR, max30102_config[i].addr, max30102_config[i].data);
    }
}


void maxim_max30102_start(void)
{

    struct reg_write max30102_config[] = {

        {REG_MODE_CONFIG, 0x03},

    };

    for (int i = 0; i < sizeof(max30102_config) / sizeof(max30102_config[0]); i++)
    {
        writeRegister8(SLAVE_ADDR, max30102_config[i].addr, max30102_config[i].data);
    }
}


void dummyOneSampleRead() {
    i2c_start(SLAVE_ADDR, WRITE);
    i2c_write(REG_FIFO_DATA);

    i2c_repeated_start(SLAVE_ADDR, READ);

    char allData[6];

    getAllBytes(1, allData);
    uint32_t tempLong = 0;
    uint32_t un_temp;

    struct reg_write max30102_config[] = {
           {REG_FIFO_WR_PTR, 0x00},
           {REG_OVF_COUNTER, 0x00},
           {REG_FIFO_RD_PTR, 0x00},
       };

   for (int i = 0; i < sizeof(max30102_config) / sizeof(max30102_config[0]); i++)
   {
       writeRegister8(SLAVE_ADDR, max30102_config[i].addr, max30102_config[i].data);
   }
}


// DMA interrupt service routine
#pragma vector=DMA_VECTOR
__interrupt void dma (void)
{
  DMA4CTL &= ~DMAIFG;
  dmaDone = 1;

}

