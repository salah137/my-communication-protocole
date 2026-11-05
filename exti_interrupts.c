#include "communication_lines/communication_line.h"
#include "stdint.h"
#include "tasks/tasks.h"
#include <stdint.h>

#define EXTI_BASE (0x40010400UL)
uint8_t decoding = 0;

// EXTI registers
#define EXTI_IMR  (*(volatile uint32_t *)(EXTI_BASE + 0x00U))
#define EXTI_RTSR (*(volatile uint32_t *)(EXTI_BASE + 0x08U))
#define EXTI_FTSR (*(volatile uint32_t *)(EXTI_BASE + 0x0CU))
#define EXTI_PR   (*(volatile uint32_t *)(EXTI_BASE + 0x14U))

// NVIC registers
#define NVIC_ISER0 (*(volatile uint32_t *)(0xE000E100UL))
#define NVIC_ICPR0 (*(volatile uint32_t *)(0xE000E280UL))
#define NVIC_IPR   ((volatile uint8_t *)(0xE000E400UL))

// RCC / AFIO / GPIOB (STM32F1)
#define RCC_APB2ENR  (*(volatile uint32_t *)(0x40021018UL))
#define AFIO_MAPR    (*(volatile uint32_t *)(0x40010004UL))
#define AFIO_EXTICR1 (*(volatile uint32_t *)(0x40010008UL))
#define AFIO_EXTICR2 (*(volatile uint32_t *)(0x4001000CUL))
#define GPIOB_CRL    (*(volatile uint32_t *)(0x40010C00UL))
#define GPIOB_ODR    (*(volatile uint32_t *)(0x40010C0CUL))

#define RCC_APB2ENR_AFIOEN  (1UL << 0)
#define RCC_APB2ENR_IOPBEN  (1UL << 3)

// EXTI1..EXTI4 -> NVIC IRQ 7..10 on F1
#define EXTI_LINES_MASK 0x1EU

extern communication_line_t **lines;
extern void Fire_PendSv(uint8_t from_isr, thread_t *t_pointer);

void set_intr_pr(uint8_t irq, uint8_t prio) {
  NVIC_IPR[irq] = (uint8_t)((prio & 0xFU) << 4);
}

static void exti_pins_init(void) {
  // PB3/PB4 are JTAG pins after reset: keep SWD, release JTAG.
  AFIO_MAPR = (AFIO_MAPR & ~(7UL << 24)) | (2UL << 24);

  // PB1..PB4: input with pull (CNF=10, MODE=00 -> 0x8), ODR=1 -> pull-up.
  // CRL nibble for pin n is at bits [4n+3:4n].
  GPIOB_CRL &= ~((0xFUL << 4) | (0xFUL << 8) | (0xFUL << 12) | (0xFUL << 16));
  GPIOB_CRL |=  ((0x8UL << 4) | (0x8UL << 8) | (0x8UL << 12) | (0x8UL << 16));
  GPIOB_ODR |=  (0xFUL << 1); // PB1..PB4 pull-up
}

void enable_exti(void) {
  RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN;

  exti_pins_init();

  // Route EXTI1..EXTI3 to port B (1 = PB) via EXTICR1 bits 4, 8, 12
  AFIO_EXTICR1 &= ~((0xFU << 4) | (0xFU << 8) | (0xFU << 12));
  AFIO_EXTICR1 |=  ((0x1U << 4) | (0x1U << 8) | (0x1U << 12));

  // Route EXTI4 to PB4 via EXTICR2 bits 0..3
  AFIO_EXTICR2 &= ~(0xFU << 0);
  AFIO_EXTICR2 |=  (0x1U << 0);

  // Edges first, unmask last, so config can't generate a spurious request
  EXTI_RTSR = EXTI_LINES_MASK;
  EXTI_FTSR = EXTI_LINES_MASK;

  set_intr_pr(7, 4);
  set_intr_pr(8, 4);
  set_intr_pr(9, 4);
  set_intr_pr(10, 4);

  // Drop anything latched while configuring
  EXTI_PR = EXTI_LINES_MASK;
  NVIC_ICPR0 |= (1U << 7) | (1U << 8) | (1U << 9) | (1U << 10);

  EXTI_IMR = EXTI_LINES_MASK;
  NVIC_ISER0 |= (1U << 7) | (1U << 8) | (1U << 9) | (1U << 10);
}

void g_handler(uint8_t i) {
  if (EXTI_PR & (1UL << i)) {
    EXTI_PR = (1UL << i); // clear first so an edge during Fire_PendSv isn't lost

    if (lines == 0 || lines[i - 1] == 0) {
      return;
    }

    thread_t *target_pointer = lines[i - 1]->communication_thread;
    Fire_PendSv(1, target_pointer);
  }
}

void EXTI_1_IRQ_handler(void) { g_handler(1); }
void EXTI_2_IRQ_handler(void) { g_handler(2); }
void EXTI_3_IRQ_handler(void) { g_handler(3); }
void EXTI_4_IRQ_handler(void) { g_handler(4); }