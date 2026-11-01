#include "stdint.h"
#include <stdlib.h>
#include "tasks/tasks.h"

extern void Init_Communication_Lines(uint8_t address);
extern int create_thread_linked(char *name, void (*thread_function)(void *),
                                uint32_t stack_size, void *params,uint8_t linked);
extern void start_routines();
extern void Fire_PendSv(uint8_t from_isr,thread_t* t_pointer);
extern void enable_exti();
extern void Systick_Init(void);
extern void TIM2_Init(void);

extern thread_t* task_bfr_isr;
extern thread_t* head;

void task1(void *param) {
  while (1) {
  }
}

void task2(void *param) {
  while (1) {
  }
}

void task3(void *param) {
  while (1) {
  }
}

void setup_tasks() {

  __asm__ volatile("cpsid i   \n\t" :::);

  create_thread_linked("task1", task1, 128, NULL,1);
  __asm__ volatile("cpsid i   \n\t" :::);

  create_thread_linked("task2", task2, 128, NULL,1);
  __asm__ volatile("cpsid i   \n\t" :::);

  create_thread_linked("task3", task3, 128, NULL,1);
  __asm__ volatile("cpsid i   \n\t" :::);

  Init_Communication_Lines(0x2A);
  
  task_bfr_isr = head;
  __asm__ volatile("cpsie i   \n\t" :::);
}

int main() {
  setup_tasks();
  start_routines();
  __asm__ volatile("mov r0,#0");
  enable_exti() ;
  TIM2_Init();
  Systick_Init();

  Fire_PendSv(0,0);

  while (1) {
  }

  return 0;
}
