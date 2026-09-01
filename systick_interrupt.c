#include "stdint.h"

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

volatile uint32_t timer_ticks = 0;

void Systick_Init(void) {
  systemt_t *SysTick = (systemt_t *)SysTick_BASE;
  uint32_t reload = 500;

  SysTick->VAL = 0;
  SysTick->LOAD = reload;

  SysTick->CTRL =
      SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE | SysTick_CTRL_CLK_SOURCE;
}

void SysTick_Handler(void){
    timer_ticks++;
    return;
}