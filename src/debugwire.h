#ifndef DEBUG_WIRE_H
#define DEBUG_WIRE_H

#include <stdint.h>

void dwInit();
bool dwEnter();
void dwExit();
uint8_t dwReadFuse(uint8_t index);

#endif//CHIP_INFO_H
