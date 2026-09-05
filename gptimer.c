// Register definitions via raw memory-mapped memory addresses
#include <stdint.h>
#define RCC_APB1ENR (*(volatile uint32_t *)(0x40023840))

#define TIM2_CR1    (*(volatile uint32_t *)(0x40000000))
#define TIM2_DIER   (*(volatile uint32_t *)(0x4000000C))
#define TIM2_SR     (*(volatile uint32_t *)(0x40000010))
#define TIM2_EGR    (*(volatile uint32_t *)(0x40000014))
#define TIM2_CNT    (*(volatile uint32_t *)(0x40000024))
#define TIM2_PSC    (*(volatile uint32_t *)(0x40000028))
#define TIM2_ARR    (*(volatile uint32_t *)(0x4000002C))

#define NVIC_ISER0 (*(volatile uint32_t *)(0xE000E100))


volatile uint32_t timer_ticks = 0;

void TIM2_Init(void) {
  RCC_APB1ENR |= (1 << 0);

  TIM2_PSC = 15;
  TIM2_ARR = 499;

  TIM2_CNT = 0;

  TIM2_EGR |= (1 << 0);
  TIM2_SR &= (1 << 0);

  TIM2_DIER |= (1 << 0);

  NVIC_ISER0 |= (1 << 28);
  TIM2_CR1 |= (1 << 0);
}

void TIM2_IRQ_handler(){
    if(TIM2_SR & (1<<0)){
        TIM2_SR &= ~(1<<0);
        timer_ticks++;
    }
}
