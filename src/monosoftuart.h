#ifndef MONOSOFTUART_H
#define MONOSOFTUART_H

#include <stdint.h>

void msuInit();
void msuBreak(uint8_t ms);
void msuSend(uint8_t data);
int msuRecv();
uint16_t msuCalibrate(); //returns 0 on failure, else the amount of timer ticks between bits.

#endif//MONOSOFTUART_H
