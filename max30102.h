
#ifndef MAX30102_H_
#define MAX30102_H_

#include <stdint.h>

void maxim_max30102_reset(void);
void maxim_max30102_init(void);
void maxim_max30102_shutdown(void);
void maxim_max30102_start(void);

void maxim_max30102_read_fifo(uint32_t *, uint32_t *);

uint8_t available(void);
void nextSample(void);
uint32_t getFIFOIR(void);
uint32_t getFIFORed(void);
uint16_t check(void);

uint32_t getIR();
uint32_t getRed();

uint8_t getReadPointer(void);
uint8_t getWritePointer(void);

void maxim_enable_interrupt(void);
#define STORAGE_SIZE 48 // Each long is 4 bytes so limit this to fit on your micro
typedef struct Record
{
    uint32_t red[STORAGE_SIZE];
    uint32_t IR[STORAGE_SIZE];
    uint32_t green[STORAGE_SIZE];
    uint32_t head;
    uint32_t tail;
} sense_struct; // This is our circular buffer of readings from the sensor

sense_struct sense;

#endif /* MAX30102_H_ */
