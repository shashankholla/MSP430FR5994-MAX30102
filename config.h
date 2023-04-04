/*
 * config.h
 *
 *  Created on: 7 Dec 2020
 *      Author: forat
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include <stdint.h>


void configClock (void);
void initTMR (void);
void delay (unsigned int);
uint32_t millis (void);


#endif /* CONFIG_H_ */
