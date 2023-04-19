/*
 * i2c.h
 *
 *  Created on: 7 Dec 2020
 *      Author: forat
 */

#ifndef I2C_H_
#define I2C_H_
#include <stdint.h>

#define READ        0
#define WRITE       1

#define STOP_I2C        (UCB0CTLW0 |= UCTXSTP)

void initI2C (void);

void i2c_start (uint8_t,unsigned int);

void i2c_stop (void);

void i2c_repeated_start(uint8_t,unsigned int);

void i2c_write (uint8_t);//write data or reg_address

void i2c_write_reg(uint8_t, uint8_t, uint8_t);

void i2c_read (uint8_t*,unsigned int);

void i2c_read2 (uint8_t*,unsigned int, int);

void i2c_read1byte(uint8_t *);
#endif /* I2C_H_ */
