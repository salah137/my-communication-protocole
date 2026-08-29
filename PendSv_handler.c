#include "stdint.h"

#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20UL))
#define SCB_ICSR (*((volatile uint32_t *)0xE000ED04UL))

#define PENDSV_PRIO_LOWEST (0xFFUL)
#define ICSR_PENDSVSET_BIT (1UL << 28)


void PendSV_Init(void){
    SCB_SHPR3 |= (PENDSV_PRIO_LOWEST << 16);
}

void Fire_PendSv(void){
    SCB_ICSR = ICSR_PENDSVSET_BIT;
}

// r2 holds the stack pointer for the wanted thread
// r1 holds the stack pointer for the old thread
__attribute__((naked)) void PendSv_handler(void) {
    __asm__ volatile(
        "cspsid i                               \n\t"
        "mrs psp,r0                             \n\t"
        "cbz rO, 1f                             \n\t"
        "stmdb r0!,{r4-r11}                     \n\t"   

        "ldr r1,=current_thread                 \n\t"
        "ldr r1,[r1]                            \n\t"
        "str r0,[r1,#8]                         \n\t"

        "1f:"
        "ldr r2,[r2]                            \n\t"
        "ldr rO,[r2,#8]                         \n\t"
        
        "ldmia r0!,{r4-r11}                     \n\t"
        "msr r0,psp                             \n\t"

        "ldr r1,=current_thread                 \n\t"
        "str r2,[r1]                            \n\t"
        "cpsie i                                \n\t"

        "ldr lr, =0xFFFFFFFD                    \n\t"   
        "bx lr                                  \n\t"                 
        : : :"memory"

    );

}
