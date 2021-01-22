#include "gpio.h"
#include "uart.h"
#include "config.h"
#include "monosoftuart.h"
#include "chipinfo.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include <stdio.h>
#include <string.h>

enum class ReceiveState : uint8_t
{
    Start,
    Sequence,
    SizeHigh,
    SizeLow,
    Token,
    Body,
    Checksum,
};

static ReceiveState receive_state = ReceiveState::Start;
static uint16_t message_size;
static uint8_t message_body[300];
static uint16_t message_body_index;
static uint8_t message_sequence_number;
static uint8_t message_checksum;
static bool message_corrupt;


static void processMessage()
{
    if (message_size < 1)
        return;
    message_body_index = 1;
    switch(message_body[0])
    {
    case 0x01: //CMD_SIGN_ON
        message_body[message_body_index++] = 0;
        message_body[message_body_index++] = 8;
        memcpy_P(&message_body[message_body_index], PSTR("STK500_2"), 8);
        message_body_index += 8;
        break;
    case 0x02: //CMD_SET_PARAMETER
        message_body[message_body_index++] = 0;
        break;
    case 0x03: //CMD_GET_PARAMETER
        //TODO: Get different parameters, not sure if it's important
        message_body[message_body_index++] = 0;
        message_body[message_body_index++] = 0;
        break;
    case 0x05: //CMD_OSCCAL
        message_body[message_body_index++] = 0;
        break;
    case 0x06: //CMD_LOAD_ADDRESS
        message_body[message_body_index++] = 0;
        break;
    case 0x10: //CMD_ENTER_PROGMODE_ISP
        message_body[message_body_index++] = 0;
        //TODO, reporting OK
        break;
    case 0x11: //CMD_LEAVE_PROGMODE_ISP
        message_body[message_body_index++] = 0;
        //TODO
        break;
    case 0x1D: //CMD_SPI_MULTI
        if (message_body[1] == 0x04 && message_body[2] == 0x04 && message_body[3] == 0x00 && message_body[4] == 0x30)
        {
            //Signature read.
            message_body[message_body_index++] = 0;
            message_body[message_body_index++] = 0x00;
            message_body[message_body_index++] = 0x00;
            message_body[message_body_index++] = 0x00;
            switch(message_body[6])
            {
            case 0x00:
                message_body[message_body_index++] = 0x1E;
                break;
            case 0x01:
                message_body[message_body_index++] = 0x91;
                break;
            case 0x02:
                message_body[message_body_index++] = 0x0A;
                break;
            default:
                message_body[message_body_index++] = 0;
            }
        }
        break;
    }
    if (message_body_index == 1)
        message_body[message_body_index++] = 0xC0;

    uartSend(0x1B);
    uartSend(message_sequence_number);
    uartSend(message_body_index >> 8);
    uartSend(message_body_index);
    uartSend(0x0E);
    uint8_t checksum = 0x1B ^ message_sequence_number ^ (message_body_index >> 8) ^ (message_body_index & 0xFF) ^ 0x0E;
    for(uint16_t idx=0; idx<message_body_index; idx++)
    {
        uartSend(message_body[idx]);
        checksum ^= message_body[idx];
    }
    uartSend(checksum);
}

int main()
{
    uartInit();
    msuInit();
    sei();

    IO_OUTPUT(B5);
    while(1)
    {
        int recv = uartGet();
        if (recv < 0)
            continue;
        IO_WRITE(B5, IO_READ(B5));
        message_checksum ^= recv;
        switch(receive_state)
        {
        case ReceiveState::Start:
            if (recv == 0x1B)
            {
                receive_state = ReceiveState::Sequence;
                message_corrupt = false;
                message_body_index = 0;
                message_checksum = recv;
            }
            break;
        case ReceiveState::Sequence:
            message_sequence_number = recv;
            receive_state = ReceiveState::SizeHigh;
            break;
        case ReceiveState::SizeHigh:
            message_size = recv << 8;
            receive_state = ReceiveState::SizeLow;
            break;
        case ReceiveState::SizeLow:
            message_size |= recv;
            receive_state = ReceiveState::Token;
            break;
        case ReceiveState::Token:
            if (recv != 0x0E)
                message_corrupt = true;
            if (message_size > sizeof(message_body))
                message_corrupt = true;
            if (message_size < 1)
                receive_state = ReceiveState::Checksum;
            else
                receive_state = ReceiveState::Body;
            break;
        case ReceiveState::Body:
            if (!message_corrupt)
                message_body[message_body_index] = recv;
            message_body_index++;
            if (message_body_index == message_size)
                receive_state = ReceiveState::Checksum;
            break;
        case ReceiveState::Checksum:
            receive_state = ReceiveState::Start;
            if (message_checksum == 0x00 && !message_corrupt)
            {
                processMessage();
            }
            break;
        }
    }
    return 0;
}
