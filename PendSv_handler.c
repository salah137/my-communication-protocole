#include "stdint.h"
#include <stdint.h>

#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20UL))
#define SCB_ICSR (*((volatile uint32_t *)0xE000ED04UL))

#define PENDSV_PRIO_LOWEST (0xFFUL)
#define ICSR_PENDSVSET_BIT (1UL << 28)

static void from_isr_handler(void);
static void from_systick_handler(void);

uint8_t isr_running = 0; // if an ISR is firing this flag should be 1 so we go out of the linked and execute the Isr thread


void PendSV_Init(void){
    SCB_SHPR3 |= (PENDSV_PRIO_LOWEST << 16);
}

void Fire_PendSv(uint8_t from_isr){
    isr_running = from_isr;
    SCB_ICSR = ICSR_PENDSVSET_BIT;
}

// r2 holds the pointer for the wanted thread
// r1 holds the pointer for the old thread
void PendSv_Handler(void) {
    if(isr_running == 1){
        from_isr_handler();
    } else {
        from_systick_handler();
    }
}

__attribute__((naked)) void from_isr_handler(void) {
  __asm__ volatile("cpsid i                 \n\t"
                   "mrs r0,psp              \n\t"
                   
                   "cbz r0,1f               \n\t"

                   "stmdb r0!,{r4-r11}      \n\t"

                   "ldr r1,=current_task    \n\t"
                   "ldr r1,[r1]             \n\t"
                   "str r0,[r1,#8]          \n\t" // change if stack placement is changed

                   "1:                      \n\t"
                   "push {lr}               \n\t"
                   "bl next_task            \n\t"
                   "pop {lr}                \n\t"
                   
                   "ldr r1,=current_task    \n\t"
                   "ldr r1,[r1]             \n\t"
                   "ldr r0,[r1,#8]          \n\t"

                   "ldmia r0!,{r4-r11}      \n\t"
                   "msr psp,r0              \n\t"
                   "cpsie i                 \n\t"

                   "ldr lr, =0xFFFFFFFD     \n\t"   
                   "bx lr                   \n\t"                 
                   : : :"memory"
                   );
}


__attribute__((naked)) void from_systick_handler(void) {
  __asm__ volatile("cpsid i                 \n\t"
                   "mrs r0,psp              \n\t"
                   
                   "cbz r0,1f               \n\t"

                   "stmdb r0!,{r4-r11}      \n\t"

                   "ldr r1,=current_task    \n\t"
                   "ldr r1,[r1]             \n\t"
                   "str r0,[r1,#8]          \n\t" // change if stack placement is changed

                   "1:                      \n\t"
                   "push {lr}               \n\t"
                   "bl next_task            \n\t"
                   "pop {lr}                \n\t"
                   
                   "ldr r1,=current_task    \n\t"
                   "ldr r1,[r1]             \n\t"
                   "ldr r0,[r1,#8]          \n\t"

                   "ldmia r0!,{r4-r11}      \n\t"
                   "msr psp,r0              \n\t"
                   "cpsie i                 \n\t"

                   "ldr lr, =0xFFFFFFFD     \n\t"   
                   "bx lr                   \n\t"                 
                   : : :"memory"
                   );
}
