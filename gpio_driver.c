#include <stdint.h>

extern void enable_exti(void);

// --- Fixed Peripheral Base Addresses ---
#define GPIOA_BASE (0x40010800UL)
#define GPIOB_BASE (0x40010C00UL)
#define RCC_BASE (0x40021000UL)
#define AFIO_BASE (0x40010000UL) // Fixed: Added trailing zero

// --- Fixed Register Macros ---
#define RCC_APB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x018U))

#define GPIOA_CRL (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))
#define GPIOA_CRH (*(volatile uint32_t *)(GPIOA_BASE + 0x04U))
#define GPIOA_IDR (*(volatile uint32_t *)(GPIOA_BASE + 0x08U))
#define GPIOA_ODR (*(volatile uint32_t *)(GPIOA_BASE + 0x0CU))
#define GPIOA_BSRR (*(volatile uint32_t *)(GPIOA_BASE + 0x10U))

#define GPIOB_CRL (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_CRH (*(volatile uint32_t *)(GPIOB_BASE + 0x04U))
#define GPIOB_IDR (*(volatile uint32_t *)(GPIOB_BASE + 0x08U))
#define GPIOB_ODR (*(volatile uint32_t *)(GPIOB_BASE + 0x0CU))
#define GPIOB_BSRR (*(volatile uint32_t *)(GPIOB_BASE + 0x10U))

#define AFIO_EXTICR1 (*(volatile uint32_t *)(AFIO_BASE + 0x08U))
#define AFIO_EXTICR2 (*(volatile uint32_t *)(AFIO_BASE + 0x0CU))
#define AFIO_EXTICR3 (*(volatile uint32_t *)(AFIO_BASE + 0x10U))
#define AFIO_EXTICR4 (*(volatile uint32_t *)(AFIO_BASE + 0x14U))

void init_gpioa() {

  for (int i = 1; i <= 4; i++) {
    GPIOA_CRL &= ~(0xFU << i * 4);
    GPIOA_CRL |= (0x1U << i * 4);
    GPIOA_ODR &= ~(1 << i);
  }
}

void init_gpiob() {
  for (int i = 1; i <= 4; i++) {
    GPIOB_CRL &= ~(0xFU << i * 4);
    GPIOB_CRL |= (0x4U << i * 4);
    GPIOB_ODR &= ~(1 << i);
  }

  AFIO_EXTICR1 = 0x1110;
  AFIO_EXTICR2 = 0x1;
}

void init_gpio() {
  RCC_APB2ENR |= (1U << 0) | (1 << 2) | (1 << 3);

  init_gpioa(); // for output lines
  init_gpiob(); // make them external interrupts

  enable_exti();
}

uint8_t read_gpiob_level(uint8_t pin) {
  if (pin > 15) {
    return -1;
  }
  return ((GPIOB_IDR & (1U << pin)) == 0) ? 0 : 1;
}

uint8_t read_gpioa_level(uint8_t pin) {
  if (pin > 15) {
    return -1;
  }
  return ((GPIOA_IDR & (1U << pin)) == 0) ? 0 : 1;
}

void set_pin_a(uint8_t pin) {
  if (pin > 15) {
    return;
  }
  GPIOA_BSRR = (1UL << pin);
}

void reset_pin_a(uint8_t pin) {
  if (pin > 15) {
    return;
  }
  GPIOA_BSRR = (1UL << (pin + 15));
}

void set_pin_b(uint8_t pin) {
  if (pin > 15) {
    return;
  }
  GPIOB_BSRR = (1UL << pin);
}

void reset_pin_b(uint8_t pin) {
  if (pin > 15) {
    return;
  }
  GPIOB_BSRR = (1UL << (pin + 15));
}
