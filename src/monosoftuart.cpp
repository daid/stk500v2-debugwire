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

//Macro to forcefully unroll specific loops for speed
#define BIT_LOOP(_M) do { _M(0); _M(1); _M(2); _M(3); _M(4); _M(5); _M(6); _M(7); } while(0)


static uint8_t bit_delay = 0;

void msuInit()
{
    HIGH();
    //Configure timer1 as 16bit up counting with F_CPU/8
    TCCR1A = 0x00;
    TCCR1B = _BV(CS11);
    TCCR1C = 0x00;
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
    uint8_t local_bit_delay = bit_delay;
    //Start bit
    LOW();
    TCNT1 = 0; while(TCNT1L < local_bit_delay) {}
    //Data
#define M(n) \
    if (data & _BV(n)) HIGH(); else LOW(); \
    TCNT1L = 0; while(TCNT1L < bit_delay) {}
    BIT_LOOP(M);
#undef M

    //Stop bit
    HIGH();
    TCNT1 = 0; while(TCNT1L < local_bit_delay) {}
}

bool msuWaitForBreak()
{
    uint16_t timeout = 0;
    while(READ()) {
        timeout++;
        if (timeout == 0) return false;
        _delay_us(50);
    }
    while(!READ())
    {
        timeout++;
        if (timeout == 0)
            return false;
        _delay_us(50);
    }
    return true;
}

int msuRecv()
{
    uint8_t local_bit_delay = bit_delay;
    //Wait for start bit
    TCNT1 = 0;
    while(READ())
    {
        if (TCNT1 > 0xF000)
            return -1; //timeout
    }
    TCNT1 = 0;
    while(TCNT1L < local_bit_delay) {}
    TCNT1 = 0;
    while(TCNT1L < local_bit_delay / 2) {}
    uint8_t result = 0;
#define M(n) \
        if (READ()) result |= _BV(n); \
        TCNT1L = 0; while(TCNT1L < local_bit_delay) {}
    BIT_LOOP(M);
#undef M
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
    while(READ()) { if (TCNT1 > 0xF000) return 0; }
    TCNT1 = 0;
    //At the start bit, wait for first bit (which is low)
    while(!READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 0, wait for bit 1
    while(READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 1, wait for bit 2
    while(!READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 2, wait for bit 3
    while(READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 3, wait for bit 4
    while(!READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 4, wait for bit 5
    while(READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 5, wait for bit 6
    while(!READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 6, wait for bit 7
    while(READ()) { if (TCNT1 > 0xF000) return 0; }
    //In bit 7, wait for stop bit
    while(!READ()) { if (TCNT1 > 0xF000) return 0; }
    //We are now in the stop bit, which is the same as line idle, so no way to detect end of stop bit.

    //t1 is now the time between 9 bits.
    uint16_t t1 = TCNT1;
    if (t1 >= 255 * 9)
        return 0;
    bit_delay = t1 / 9;

    //Wait for the stop bit to finish.
    TCNT1 = 0; while(TCNT1 < bit_delay) {}
    return bit_delay;
}
