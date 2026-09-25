#include "stdint.h"

extern void Init_Communication_Lines(uint8_t address);

int main(){
    Init_Communication_Lines(0x03);
    
    while (1) {
    
    }

    return 0;
}