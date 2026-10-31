#include "dump_heap/dump_heap.h"
#include <stdint.h>

extern uint32_t _sidata;
extern uint32_t _estack;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern void enable_exti();


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
extern void SysTick_Handler(void);

// TIM2
extern void TIM2_Init(void);
extern void TIM2_IRQHandler(void);

// PendSv
extern void PendSV_Init(void);
extern void PendSv_Handler(void);
extern int main();
extern void Init_Communication_Lines(void);

extern void init_gpio() ;


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

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);

#define IRQ(n) [16 + (n)]

__attribute__((section(".vector_table"))) const uint32_t vector_table[] = {
    [0]  = (uint32_t)&_estack,
    [1]  = (uint32_t)&Reset_Handler,
    [2]  = (uint32_t)&NMI_Handler,
    [3]  = (uint32_t)&HardFault_Handler,
    [11] = (uint32_t)&SVC_Handler,
    [14] = (uint32_t)&PendSv_Handler,
    [15] = (uint32_t)&SysTick_Handler,

    [16 ... 16 + 42] = (uint32_t)&Default_Handler,

    IRQ(7)  = (uint32_t)&EXTI_1_IRQ_handler,
    IRQ(8)  = (uint32_t)&EXTI_2_IRQ_handler,
    IRQ(9)  = (uint32_t)&EXTI_3_IRQ_handler,
    IRQ(10) = (uint32_t)&EXTI_4_IRQ_handler,
    IRQ(28) = (uint32_t)&TIM2_IRQHandler,
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

  init_gpio();

  PendSV_Init();
  main();
  while (1) {
  
  };
}

void NMI_Handler(void) {
  while (1) {
  }
}

void Default_Handler(void) {
  while (1) {
  }
}

__attribute__((naked)) void HardFault_Handler(void) {
  __asm volatile(
    "tst lr, #4      \n"
    "ite eq          \n"
    "mrseq r0, msp   \n"
    "mrsne r0, psp   \n"
    "b hardfault_c   \n");
}

void hardfault_c(uint32_t *frame) {
    
  volatile uint32_t pc   = frame[6];
  volatile uint32_t lr   = frame[5];
  volatile uint32_t cfsr = *(volatile uint32_t *)0xE000ED28;
  volatile uint32_t bfar = *(volatile uint32_t *)0xE000ED38;
  (void)pc; (void)lr; (void)cfsr; (void)bfar;
  while (1) { }
}

__attribute__((naked)) void SVC_Handler(void) {
  __asm__ volatile("isb                       \n\t"
                   "ldr lr, =0xFFFFFFF9      \n\t"
                   "bx lr                    \n\t" ::
                       :);
}
