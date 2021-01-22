#include "monosoftuart.h"
#include "config.h"
#include "gpio.h"
#include "uart.h"

#include <util/delay.h>
#include <stdio.h>
#include <string.h>


#define HIGH() do { IO_INPUT(RST_PIN); IO_WRITE(RST_PIN, 1); } while(0)
#define LOW() do { IO_WRITE(RST_PIN, 0); IO_OUTPUT(RST_PIN); } while(0)
#define READ() IO_READ(RST_PIN)


static uint16_t bit_delay = 0;

void msuInit()
{
    HIGH();
    //Configure timer1 as 16bit up counting with F_CPU/8
    TCCR1A = 0x00;
    TCCR1B = _BV(CS11);
    TCCR1C = 0x00;

IO_OUTPUT(B3);
}

void msuBreak(uint8_t ms)
{
    LOW();
    while(ms--)
        _delay_ms(1);
    HIGH();
}

void msuSend(uint8_t data)
{
    //Start bit
IO_WRITE(B3, !IO_READ(B3));
    LOW();
    TCNT1 = 0; while(TCNT1 < bit_delay) {}
    //Data
    for(uint8_t n=0; n<8; n++)
    {
        if (data & _BV(n)) HIGH(); else LOW();
IO_WRITE(B3, !IO_READ(B3));
        TCNT1 = 0; while(TCNT1 < bit_delay) {}
    }

    //Stop bit
    HIGH();
IO_WRITE(B3, !IO_READ(B3));
    TCNT1 = 0; while(TCNT1 < bit_delay) {}
}

int msuRecv()
{
    //Wait for start bit
    TCNT1 = 0;
    while(READ())
    {
        if (TCNT1 > 0xF000)
            return -1; //timeout
    }
    TCNT1 = 0;
    while(TCNT1 < bit_delay) {}
    TCNT1 = 0;
    while(TCNT1 < bit_delay / 2) {}
    uint8_t result = 0;
    for(uint8_t n=0; n<8; n++)
    {
IO_WRITE(B3, !IO_READ(B3));
        if (READ()) result |= _BV(n);
        TCNT1 = 0;
        while(TCNT1 < bit_delay) {}
    }
    // Check the stop bit.
    if (!READ())
        return -1;
    return result;
}

uint16_t msuCalibrate()
{
    //Expect a 0x55 byte, at an unknown baudrate.
    //Wait for the start bit
    TCNT1 = 0;
    while(READ())
    {
        if (TCNT1 > 0xF000)
            return 0;   //timeout
    }
    TCNT1 = 0;
    //At the start bit, wait for first bit (which is low)
    while(!READ()) {}
    //In bit 0, wait for bit 1
    while(READ()) {}
    //In bit 1, wait for bit 2
    while(!READ()) {}
    //In bit 2, wait for bit 3
    while(READ()) {}
    //In bit 3, wait for bit 4
    while(!READ()) {}
    //In bit 4, wait for bit 5
    while(READ()) {}
    //In bit 5, wait for bit 6
    while(!READ()) {}
    //In bit 6, wait for bit 7
    while(READ()) {}
    //In bit 7, wait for stop bit
    while(!READ()) {}
    //We are now in the stop bit.

    //t1 is now the time between 9 bits.
    uint16_t t1 = TCNT1;
    bit_delay = t1 / 9;

    //Wait for the stop bit to finish.
    TCNT1 = 0; while(TCNT1 < bit_delay) {}
    return bit_delay;
}
