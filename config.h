/*
 * config.h
 *
 *  Created on: 7 Dec 2020
 *      Author: forat
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include <stdint.h>

void dmaConfig(void);
void configClock (void);
void initTMR (void);
void delay (unsigned int);
uint32_t millis (void);
void disableUnwantedGPIO(void);

#endif /* CONFIG_H_ */
