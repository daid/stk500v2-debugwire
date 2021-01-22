#include "chipinfo.h"

#include <avr/pgmspace.h>


ChipInfo chip_info;

static const ChipInfo chip_info_list[] PROGMEM = {
    //signature, flash_size, page_size, DWDR, name[16];
    { 0x910A,    2048,       32,        0x1F, "TINY2313A"},
};

bool loadChipData(uint16_t chip_id)
{
    for(size_t n=0; n<sizeof(chip_info_list)/sizeof(chip_info_list[0]); n++)
    {
        if (chip_id == pgm_read_word(&chip_info_list[n].signature))
        {
            memcpy_P(&chip_info, &chip_info_list[n], sizeof(ChipInfo));
            return true;
        }
    }
    return false;
}
