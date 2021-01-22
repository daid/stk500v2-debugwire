#include "uart.h"
#include "gpio.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


static uint8_t recv_buffer[256];
static uint8_t volatile recv_write;
static uint8_t volatile recv_read;


//TODO: This assumes USART0, could use macros to configure which UART to use.
void uartInit()
{
    UCSR0A = _BV(U2X0);
    UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

    UBRR0 = ((F_CPU + (4 * UART_BAUD)) / (8 * UART_BAUD)) - 1;

    IO_OUTPUT(UART_TX_PIN);
    IO_INPUT(UART_RX_PIN);
}

void uartSend(uint8_t data)
{
    while((UCSR0A & _BV(UDRE0)) == 0) {}
    UDR0 = data;
}

void uartWrite(const void* data, uint8_t length)
{
    while(length)
    {
        while((UCSR0A & _BV(UDRE0)) == 0) {}
        UDR0 = *(uint8_t*)data;
        length -= 1;
        data = ((uint8_t*)data) + 1;
    }
}

void uartWrite_P(const void* data, uint8_t length)
{
    while(length)
    {
        while((UCSR0A & _BV(UDRE0)) == 0) {}
        UDR0 = pgm_read_byte(data);
        length -= 1;
        data = ((uint8_t*)data) + 1;
    }
}

int uartGet()
{
    if (recv_write == recv_read)
        return -1;
    return recv_buffer[recv_read++];
}

ISR(USART_RX_vect)
{
    recv_buffer[recv_write] = UDR0;
    recv_write++;
}
