#ifndef CHIP_INFO_H
#define CHIP_INFO_H

#include <stdint.h>

struct ChipInfo
{
    uint16_t signature;
    uint32_t flash_size;
    uint16_t page_size;
    uint8_t  DWDR;
    char     name[16];
};
extern ChipInfo chip_info;

bool loadChipData(uint16_t chip_id);

#endif//CHIP_INFO_H
