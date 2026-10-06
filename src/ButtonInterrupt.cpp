#include "ButtonInterrupt.h"

/*
Description from the internet:
& ~MASK → wipe the field (keep everything else)
& MASK → trim the new value to fit the field
| → drop the trimmed value into the wiped field
*/

void ButtonInterrupt::init(){

}

void setDataDirection(bool isInput){
    if (isInput){
        uint32_t tmp = READ_GPIO_EMC_07();
        tmp &= ~0xF;
        tmp |= ((1 << 4) & 0xF);
        WRITE_GPIO_EMC_07(tmp);
    }

    uint32_t tmp = READ_GPIO_EMC_07();
    tmp &= ~0xF;
    tmp |= ((0 << 4) & 0xF);
    WRITE_GPIO_EMC_07(tmp);
}

void setIOMUX_GPIO_MODE(){
    uint32_t tmp = READ_GPIO_EMC_07();
    tmp &= ~0x8;
    tmp |= ((5 << 0) & 0x6);
    WRITE_GPIO_EMC_07(tmp);
}