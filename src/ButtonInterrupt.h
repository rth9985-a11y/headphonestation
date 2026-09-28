#define IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_07 0x401F8030
#define READ_GPIO_EMC_07() (*(volatile uint32_t* )IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_07)
#define WRITE_GPIO_EMC_07(val) (*(volatile uint32_t* )IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_07 = (val))

#include <stdint.h>

class ButtonInterrupt{

    void init();
    void setDataDirection(bool isInput);
    void setIOMUXType(uint32_t mux_mode);

};