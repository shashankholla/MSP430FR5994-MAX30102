#include <msp430.h>
#include <stdint.h>
#include "max30102.h"
#include "i2c.h"

#define SLAVE_ADDR          0x57
//register addresses
#define REG_INTR_STATUS_1   0x00
#define REG_INTR_STATUS_2   0x01
#define REG_INTR_ENABLE_1   0x02
#define REG_INTR_ENABLE_2   0x03
#define REG_FIFO_WR_PTR     0x04
#define REG_OVF_COUNTER     0x05
#define REG_FIFO_RD_PTR     0x06
#define REG_FIFO_DATA       0x07
#define REG_FIFO_CONFIG     0x08
#define REG_MODE_CONFIG     0x09
#define REG_SPO2_CONFIG     0x0A
#define REG_LED1_PA         0x0C
#define REG_LED2_PA         0x0D
#define REG_PILOT_PA        0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12
#define REG_TEMP_INTR       0x1F
#define REG_TEMP_FRAC       0x20
#define REG_TEMP_CONFIG     0x21
#define REG_PROX_INT_THRESH 0x30
#define REG_REV_ID          0xFE
#define REG_PART_ID         0xFF

 struct reg_write {
        uint8_t addr;
        uint8_t data;
    };

void writeRegister8(uint8_t address, uint8_t reg, uint8_t value) {
    i2c_start(address,WRITE);
    i2c_write(reg);
    i2c_write(value);
    i2c_stop();
}

void maxim_max30102_reset (void)
{
//     //S SLA W ACK 0x09 ACK 0x40 ACK P
//     //shutdown IC for LPM
//     i2c_start (SLAVE_ADDR,WRITE);
//     i2c_write(REG_MODE_CONFIG);
//     i2c_write(0x40);//shut down IC for LPM usage
//     i2c_stop ();
writeRegister8(SLAVE_ADDR, REG_MODE_CONFIG, 0x40); // Reset
}

void maxim_max30102_init (void)
{
    struct reg_write max30102_config[] = {
                                          {REG_INTR_ENABLE_1, 0x00},
                                          {REG_INTR_ENABLE_2, 0x00},
                                          {REG_FIFO_WR_PTR,   0x00},
                                          {REG_OVF_COUNTER, 0x00},
                                          {REG_FIFO_RD_PTR, 0x00},
                                          {REG_FIFO_CONFIG, 0x5F},
                                          {REG_MODE_CONFIG, 0x07},
                                          {REG_SPO2_CONFIG, 0x2F},
                                          {REG_LED1_PA, 0x0A},
                                          {REG_LED2_PA, 0x1F},
                                          {REG_LED2_PA+1, 0x1F},
                                          {REG_LED2_PA+2, 0x1F},
                                          {REG_LED2_PA+3, 0x1F},
                                          {REG_MULTI_LED_CTRL1, 0x21},
                                          {REG_MULTI_LED_CTRL2, 0x03},
    };
    uint8_t addr [11] = {REG_INTR_ENABLE_1,REG_INTR_ENABLE_2,REG_FIFO_WR_PTR,REG_OVF_COUNTER,REG_FIFO_RD_PTR,REG_FIFO_CONFIG,0x03,0x27,0x24,0x24,0x7F};
    uint8_t data [11] = {0x00,0x00,0x00,0x00,0x00,0x4F,0x03,0x27,0x24,0x24,0x7F};

    for(int i=0; i < sizeof(max30102_config)/sizeof(reg_write); i++) {
        writeRegister8(SLAVE_ADDR, max30102_config[i].addr, max30102_config[i].data);
    }
}

void maxim_max30102_read_fifo (uint32_t* ptr_red_led, uint32_t * ptr_ir_led)
{
    uint32_t un_temp;

    *ptr_ir_led  = 0;
    *ptr_red_led = 0;

    i2c_start(SLAVE_ADDR,WRITE);
    i2c_write(REG_FIFO_DATA);

    //i2c_start(SLAVE_ADDR,READ);
    i2c_repeated_start(SLAVE_ADDR,READ);

    uint8_t led[6] = {0,0,0,0,0,0};
    unsigned int RxByteCtr = 6;

    i2c_read(led,RxByteCtr);

    un_temp = led[0];
    un_temp <<= 16;
    *ptr_red_led += un_temp;

    un_temp = led[1];
    un_temp <<= 8;
    *ptr_red_led += un_temp;

    un_temp = led[2];
    *ptr_red_led += un_temp;

    un_temp = 0;

    un_temp = led[3];
    un_temp <<= 16;
    *ptr_ir_led += un_temp;

    un_temp = led[4];
    un_temp <<= 8;
    *ptr_ir_led += un_temp;

    un_temp = led[5];
    *ptr_ir_led += un_temp;
}
