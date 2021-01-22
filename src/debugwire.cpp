#include "debugwire.h"
#include "monosoftuart.h"
#include "chipinfo.h"
#include "config.h"

#include <avr/sfr_defs.h>

#define DW_READ_SIG 0xF3

static void dwInstr(uint16_t instr)
{
    msuSend(0x64);
    msuSend(0xD2);
    msuSend(instr >> 8);
    msuSend(instr);
    msuSend(0x23);
}
#define LDI(reg, value) \
    dwInstr(0xE000 | (((value) & 0xF0) << 4) | ((reg & 0x0F) << 4) | ((value) & 0x0F))
#define IN(addr, reg) \
    dwInstr(0xB000 | (((addr) & 0x30) << 5) | (((reg) & 0x1F) << 4) | ((addr) & 0x0F))
#define OUT(addr, reg) \
    dwInstr(0xB800 | (((addr) & 0x30) << 5) | (((reg) & 0x1F) << 4) | ((addr) & 0x0F))
#define LPM_Z(reg) \
    dwInstr(0x9004 | (((reg) & 0x1F) << 4))

#define SET_Z(value) \
    do { LDI(30, (value) & 0xFF); LDI(31, (value) >> 8); } while(0)

void dwInit()
{
    msuInit();
}

bool dwEnter()
{
    msuBreak(10);
    uint16_t bit_delay = msuCalibrate();
    if (bit_delay == 0)
        return false;
    msuSend(DW_READ_SIG);
    int sigHigh = msuRecv();
    if (sigHigh < 0)
        return false;
    int sigLow = msuRecv();
    if (sigLow < 0)
        return false;
    return loadChipData((sigHigh << 8) | sigLow);
}

void dwExit()
{
    msuSend(0xD0);
    msuSend(0x00);
    msuSend(0x00);
    msuSend(0x60);
    msuSend(0x30);
}

uint8_t dwReadFuse(uint8_t index)
{
    SET_Z(index);
    LDI(16, _BV(0) | _BV(3));
    OUT(0x37, 16);
    LPM_Z(16);
    OUT(chip_info.DWDR, 16);
    return msuRecv();
}
