#ifndef DEBUG_WIRE_H
#define DEBUG_WIRE_H

#include <stdint.h>

void dwInit();
bool dwEnter();
bool dwExit();

uint8_t dwReadFuse(uint8_t index);

bool dwChipErase();
bool dwReadFlash(uint32_t address, uint8_t* data, uint16_t size);
bool dwWriteFlash(uint32_t address, const uint8_t* data, uint16_t size);

#endif//CHIP_INFO_H
