#include "communication_lines/communication_line.h"
#include "stdint.h"
#include <stdint.h>

#define EXTI_BASE (0x40010400UL)

// EXTI Peripheral Registers
#define EXTI_IMR                                                               \
  (*(volatile uint32_t *)(EXTI_BASE + 0x00U)) // Interrupt Mask Register
#define EXTI_RTSR                                                              \
  (*(volatile uint32_t *)(EXTI_BASE +                                          \
                          0x08U)) // Rising Trigger Selection Register
#define EXTI_FTSR                                                              \
  (*(volatile uint32_t *)(EXTI_BASE +                                          \
                          0x0CU)) // Falling Trigger Selection Register
#define EXTI_PR (*(volatile uint32_t *)(EXTI_BASE + 0x14U)) // Pending Register

// NVIC Registers (Fixed syntax errors)
#define NVIC_ISER0 (*(volatile uint32_t *)(0xE000E100UL))
#define NVIC_ISER1 (*(volatile uint32_t *)(0xE000E104UL))
#define NVIC_IPR ((volatile uint8_t *)(0xE000E400UL))

#define RCC_APB2ENR (*(volatile uint32_t *)(0x40021018UL))
#define AFIO_EXTICR1 (*(volatile uint32_t *)(0x40010008UL))
#define AFIO_EXTICR2 (*(volatile uint32_t *)(0x4001000CUL))

extern communication_line_t **lines;
extern void Fire_PendSv(uint8_t from_isr);

void set_intr_pr(uint8_t irq, uint8_t priot) {
  NVIC_IPR[irq] = (uint8_t)(priot & 0xFU << 4);
}

void enable_exti() {
  RCC_APB2ENR |= (1UL << 0);

  AFIO_EXTICR1 &= ~((0xFU << 4) | (0xFU << 8) | (0xFU << 12));
  AFIO_EXTICR1 |= ((0x1U << 4) | (0x1U << 8) | (0x1U << 12));

  AFIO_EXTICR2 &= ~(0xFU << 0);
  AFIO_EXTICR2 |=  (0x1U << 0);


  
  EXTI_IMR = (0x1EU);
  EXTI_RTSR = (0x1EU);
  EXTI_FTSR = (0x1EU);

  set_intr_pr(7, 4);
  set_intr_pr(8, 4);
  set_intr_pr(9, 4);
  set_intr_pr(10, 4);

  NVIC_ISER0 = (1U << 7) | (1U << 8) | (1U << 9) | (1U << 10);
}

void g_handler(uint8_t i) {
  if (EXTI_PR & (1UL << i)) {
    uint32_t *target_pointer = (uint32_t *)((uint32_t *)lines + (i - 1));

    __asm__ volatile("ldr r2,[%0]                   \n\t"
                     "ldr r2,[r2]                   \n\t"
                     :
                     : "r"(target_pointer)
                     : "memory");

    Fire_PendSv(1);

    EXTI_PR = (1 << i);
  }
}

void EXTI_1_IRQ_handler(void) { g_handler(1); }

void EXTI_2_IRQ_handler(void) { g_handler(2); }

void EXTI_3_IRQ_handler(void) { g_handler(3); }
void EXTI_4_IRQ_handler(void) { g_handler(4); }
