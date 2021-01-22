#ifndef UART_H
#define UART_H

#include <stdint.h>

void uartInit();
void uartSend(uint8_t data);
void uartWrite(const void* data, uint8_t size);
void uartWrite_P(const void* data, uint8_t size);
int uartGet();

#endif//UART_H
