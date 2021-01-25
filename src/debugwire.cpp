#include "debugwire.h"
#include "monosoftuart.h"
#include "chipinfo.h"
#include "config.h"

#include <avr/sfr_defs.h>
#include <util/delay.h>

static uint8_t current_mode = 0;


static bool dwSync()
{
    uint16_t bit_delay = msuCalibrate();
    if (bit_delay == 0)
        return false;
    uint8_t speed_setting = 0x83;
    while(bit_delay > 50 && speed_setting >= 0x80)
    {
        speed_setting -= 1;
        msuSend(speed_setting);
        bit_delay = msuCalibrate();
        if (bit_delay == 0)
            return false;
    }
    return true;
}

//Setup and execute an instruction
static void dwInstr(uint16_t instr)
{
    if (current_mode != 0x64)
    {
        msuSend(0x64);
        current_mode = 0x64;
    }
    msuSend(0xD2);
    msuSend(instr >> 8);
    msuSend(instr);
    msuSend(0x23);
}

//Setup and execute an instruction, AND generate a break+0x55 after the instruction is done.
static bool dwInstrSync(uint16_t instr)
{
    if (current_mode != 0x64)
    {
        msuSend(0x64);
        current_mode = 0x64;
    }
    msuSend(0xD2);
    msuSend(instr >> 8);
    msuSend(instr);
    msuSend(0x33);

    if (!msuWaitForBreak())
        return false;
    if (dwSync() == 0)
        return false;
    return true;
}

static void dwSetPC(uint16_t value)
{
    msuSend(0xD0);
    msuSend(value >> 8);
    msuSend(value);
}

static void dwSetBP(uint16_t value)
{
    msuSend(0xD1);
    msuSend(value >> 8);
    msuSend(value);
}

#define LDI(reg, value) \
    dwInstr(0xE000 | (((value) & 0xF0) << 4) | ((reg & 0x0F) << 4) | ((value) & 0x0F))
#define ADIW_Z(value) \
    dwInstr(0x9630 | ((value & 0x30) << 2) | (value & 0x0F))
#define IN(addr, reg) \
    dwInstr(0xB000 | (((addr) & 0x30) << 5) | (((reg) & 0x1F) << 4) | ((addr) & 0x0F))
#define OUT(addr, reg) \
    dwInstr(0xB800 | (((addr) & 0x30) << 5) | (((reg) & 0x1F) << 4) | ((addr) & 0x0F))
#define LPM_Z(reg) \
    dwInstr(0x9004 | (((reg) & 0x1F) << 4))
#define SPM() \
    dwInstr(0x95E8)
#define SPM_SYNC() \
    dwInstrSync(0x95E8)

#define SET_Z(value) \
    do { LDI(30, (value) & 0xFF); LDI(31, (value) >> 8); } while(0)

void dwInit()
{
    msuInit();
}

bool dwEnter()
{
    msuBreak(10);
    dwSync();
    msuSend(0xF3);
    int sigHigh = msuRecv();
    if (sigHigh < 0)
        return false;
    int sigLow = msuRecv();
    if (sigLow < 0)
        return false;
    return loadChipData((sigHigh << 8) | sigLow);
}

bool dwExit()
{
    msuSend(0x07);
    //Wait for break, and the break to end (can take quite a while, 70ms has been seen)
    if (!msuWaitForBreak())
        return false;
    //After this a 0x55 is send by the chip, which could be a different baudrate
    if (msuCalibrate() == 0)
        return false;
    //Set the PC to 0x0000 and run the main code.
    dwSetPC(0x0000);
    msuSend(0x60);
    current_mode = 0x60;
    msuSend(0x30);
    return true;
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

bool dwChipErase()
{
    uint32_t address = 0;
    dwSetPC(chip_info.flash_size - 2);
    LDI(16, _BV(0) | _BV(1));
    while(address < chip_info.flash_size)
    {
        SET_Z(address);
        OUT(0x37, 16);
        if (!SPM_SYNC())
            return false;
        address += chip_info.page_size;
    }
    return true;
}

bool dwReadFlash(uint32_t address, uint8_t* data, uint16_t size)
{
    SET_Z(address * 2);
    dwSetPC(0x0000);
    dwSetBP(size * 2);
    msuSend(0x66);
    current_mode = 0x66;
    msuSend(0xC2); msuSend(0x02);
    msuSend(0x20);
    while(size)
    {
        int recv = msuRecv();
        if (recv < 0)
            return false;
        *data = recv;
        data++;
        size--;
    }
    return true;
}

bool dwWriteFlash(uint32_t address, const uint8_t* data, uint16_t size)
{
    LDI(16, _BV(0) | _BV(4));
    OUT(0x37, 16);
    SPM();

    LDI(16, _BV(0));
    dwSetPC(chip_info.flash_size - 2);
    SET_Z(address * 2);
    while(size)
    {
        IN(chip_info.DWDR, 0); msuSend(*data++);
        IN(chip_info.DWDR, 1); msuSend(*data++);
        OUT(0x37, 16);
        SPM();
        ADIW_Z(2);
        size -= 2;
    }
    
    SET_Z(address * 2);
    LDI(16, _BV(0) | _BV(2));
    OUT(0x37, 16);
    if (!SPM_SYNC())
        return false;
    return true;
}
