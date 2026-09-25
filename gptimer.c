#include <stdint.h>

// Corrected base addresses and offsets for STM32F103
#define RCC_APB1ENR (*(volatile uint32_t *)(0x4002101C))

#define TIM2_CR1    (*(volatile uint32_t *)(0x40000000))
#define TIM2_DIER   (*(volatile uint32_t *)(0x4000000C))
#define TIM2_SR     (*(volatile uint32_t *)(0x40000010))
#define TIM2_EGR    (*(volatile uint32_t *)(0x40000014))
#define TIM2_CNT    (*(volatile uint32_t *)(0x40000024))
#define TIM2_PSC    (*(volatile uint32_t *)(0x40000028))
#define TIM2_ARR    (*(volatile uint32_t *)(0x4000002C))

#define NVIC_ISER0  (*(volatile uint32_t *)(0xE000E100))

volatile uint32_t timer_ticks = 0;

void TIM2_Init(void) {
  // Enable TIM2 clock on APB1 for STM32F1 (Bit 0)
  RCC_APB1ENR |= (1 << 0);

  TIM2_PSC = 15;
  TIM2_ARR = 499;

  TIM2_CNT = 0;

  TIM2_EGR |= (1 << 0);
  
  TIM2_SR &= ~(1 << 0);

  TIM2_DIER |= (1 << 0); // Enable update interrupt

  // Enable TIM2 global interrupt in NVIC (Position 28 for TIM2 on STM32F1)
  NVIC_ISER0 |= (1 << 28);
  
  TIM2_CR1 |= (1 << 0);  // Enable counter
}

void TIM2_IRQHandler(void) {
    if (TIM2_SR & (1 << 0)) {
        TIM2_SR &= ~(1 << 0); // Clear update interrupt flag safely
        timer_ticks++;
    }
}