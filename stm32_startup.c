#include "dump_heap/dump_heap.h"
#include <stdint.h>

extern uint32_t _sidata;
extern uint32_t _estack;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;


uint8_t my_dump_heap[4098] __attribute__((aligned(8))) = {0};

my_heap_t my_heap = {
    .start = &my_dump_heap[0],
    .end = &my_dump_heap[0] + sizeof(my_dump_heap),
    .pos = &my_dump_heap[0],
};

// EXTI_INTERRUPTS_HANDLERS
extern void EXTI_1_IRQ_handler();
extern void EXTI_2_IRQ_handler();
extern void EXTI_3_IRQ_handler();
extern void EXTI_4_IRQ_handler();
extern void EXTI_5_9_IRQ_handler();
extern void EXTI_10_15_IRQ_handler();

// Systick 
extern void Systick_Init(void);
extern void SysTick_Handler(void);

// PendSv
extern void PendSV_Init(void);
extern void PendSv_Handler(void);

extern void Init_Communication_Lines(void);

typedef struct {
  uint32_t CTRL;
  uint32_t LOAD;
  uint32_t VAL;
  uint32_t CALIB;
} systemt_t;

#define SysTick_BASE (0xE000E010UL)

#define SysTick_CTRL_ENABLE (1UL << 0)
#define SysTick_CTRL_TICKINT (1UL << 1)
#define SysTick_CTRL_CLK_SOURCE (1UL << 2)


void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void) __attribute((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute((weak, alias("Default_Handler")));
void SVC_Handler(void);

__attribute__((section(".vector_table"))) const uint32_t vector_table[] = {
    (uint32_t) &_estack,
    (uint32_t)&Reset_Handler,     // Index 1 : Reset Handler
    (uint32_t)&NMI_Handler,       // Index 2 : NMI
    (uint32_t)&HardFault_Handler, // Index 3 : HardFault
    0,
    0,
    0,
    0,
    0,
    0,
    0, // Index 4-10 : Faults & Reserved
    (uint32_t)&SVC_Handler, // Index 11 : SVCall
    0,
    0,                          // Index 12-13 : Debug / Reserved
    (uint32_t)&PendSv_Handler,  // Index 14 : PendSV
    (uint32_t)&SysTick_Handler, // Index 15 : SysTick
    (uint32_t)&Default_Handler, // Index 16 : WWDG (IRQ 0)
    (uint32_t)&Default_Handler, // Index 17 : PVD (IRQ 1)
    (uint32_t)&Default_Handler, // Index 18 : TAMPER (IRQ 2)
    (uint32_t)&Default_Handler, // Index 19 : RTC (IRQ 3)
    (uint32_t)&Default_Handler, // Index 20 : FLASH (IRQ 4)
    (uint32_t)&Default_Handler, // Index 21 : RCC (IRQ 5)

    (uint32_t)&Default_Handler, 
    (uint32_t)&EXTI_1_IRQ_handler, // Index 22 : EXTI0 (IRQ 6)
    (uint32_t)&EXTI_2_IRQ_handler, // Index 23 : EXTI1 (IRQ 7)
    (uint32_t)&EXTI_3_IRQ_handler, // Index 24 : EXTI2 (IRQ 8)
    (uint32_t)&EXTI_4_IRQ_handler, // Index 25 : EXTI3 (IRQ 9)

    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 

    0,

    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 
    (uint32_t)&Default_Handler, 

    0
};

void Reset_Handler(void) {
  uint32_t *_sptr = &_sidata;
  uint32_t *d_ptr = &_sdata;

  while (d_ptr < &_edata) {
    (*d_ptr++) = *(_sptr++);
  }

  d_ptr = &_sbss;

  while (d_ptr < &_ebss) {
    (*d_ptr++) = 0;
  }
  
  Systick_Init();
  PendSV_Init();
  
  
}

void Default_Handler(){
    while (1) {
    
    }
}

__attribute__((naked)) void SVC_Handler(void){
    __asm__ volatile (
            "mrs r0, control   \n\t" // Read CONTROL register
            "bic r0, r0, #2    \n\t" // Clear bit 1 (SPSEL = 0 -> use MSP)
            "msr control, r0   \n\t" // Write back to CONTROL
            "isb               \n\t" // Instruction Synchronization Barrier
            ::: "memory"
        );   
}