#include "stdint.h"
#include "tasks/tasks.h"
#include <stdint.h>

#define SCB_SHPR3 (*((volatile uint32_t *)0xE000ED20UL))
#define SCB_ICSR (*((volatile uint32_t *)0xE000ED04UL))

#define PENDSV_PRIO_LOWEST (0xFFUL)
#define ICSR_PENDSVSET_BIT (1UL << 28)

extern  thread_t *head;
extern uint8_t decoding;
extern thread_t* task_bfr_isr;
extern thread_t* current_task;

static void from_isr_handler(void);
static void from_systick_handler(void);
volatile int hit = 0;

uint8_t isr_running = 0; // if an ISR is firing this flag should be 1 so we go
                         // out of the linked and execute the Isr thread

uint8_t isr_running_last = 0;
                         
volatile thread_t *th_pointer = 0;

uint8_t first_sch = 0;
uint8_t first_sch1 = 0;

void PendSV_Init(void) { SCB_SHPR3 |= (PENDSV_PRIO_LOWEST << 16); }

static void holy1(){
    while (1) {
    
    }
}


static void holy2(){
    while (1) {
    
    }
}

void Fire_PendSv(uint8_t from_isr, thread_t* t_pointer) {
  if(t_pointer !=0){
      th_pointer = t_pointer;
      
  }
  isr_running = from_isr;
  SCB_ICSR = ICSR_PENDSVSET_BIT;
  __asm__ volatile("dsb; isb");
}




void PendSv_Handler(void) {

  __asm__ volatile("cpsid i                 \n\t" :::);
  if (isr_running == 1) {
      if(isr_running_last == 0){
          task_bfr_isr = current_task;
      }
    isr_running_last = 1;
    
    if (first_sch1 == 0) {
      __asm__ volatile("mov r0,#0      \n\t" :::);

      first_sch1 = 1;
    } else {
        
      __asm__ volatile("mov r0,#1      \n\t" :::);
    }

    
    __asm__ volatile("mov r2,%0                       \n\t"
                    // "ldr r2,[r2]                   \n\t"
                     :
                     : "r"(th_pointer)
                     : "memory");
    
    from_isr_handler();
    __asm__ volatile("cpsie i                 \n\t"

                     "ldr lr, =0xFFFFFFFD     \n\t"
                     "bx lr                   \n\t" ::
                         :);

  } else {
    isr_running_last = 0;
    if (first_sch == 0) {
      __asm__ volatile("mov r0,#0      \n\t" :::);

      first_sch = 1;
    } else {
      __asm__ volatile("mov r0,#1      \n\t" :::);
    }

    from_systick_handler();
    __asm__ volatile("cpsie i                 \n\t"

                     "ldr lr, =0xFFFFFFFD     \n\t"
                     "bx lr                   \n\t" ::
                         :);
  }

}

__attribute__((naked)) void from_isr_handler(void) {
  __asm__ volatile("pop {r7,lr}                  \n\t"
                   "cpsid i                 \n\t"
                   "cbz r0,1f               \n\t"

                   "mrs r0,psp              \n\t"

                   "stmdb r0!,{r4-r11}      \n\t"

                   "ldr r1,=current_task    \n\t"
                   "ldr r1,[r1]             \n\t"
                   "str r0,[r1,#8]          \n\t"

                   "1:                      \n\t"
                   "ldr r2, =th_pointer     \n\t"
                   "ldr r2,[r2]             \n\t"
                   "ldr r1, =current_task   \n\t"
                   "str r2,[r1]          \n\t"
                   
                   "ldr r1,=current_task      \n\t"
                   "ldr r1,[r1]             \n\t"
                   "ldr r0,[r1,#8]          \n\t"

                   "ldmia r0!,{r4-r11}      \n\t"
                   "msr psp,r0              \n\t"

                   "cpsie i                 \n\t"

                   "ldr lr, =0xFFFFFFFD     \n\t"
                   "bx lr                   \n\t"
                   :
                   :
                   : "memory");
}

__attribute__((naked)) void from_systick_handler(void) {
  __asm__ volatile(
      "pop {r7,lr}            \n\t"

      "cpsid i                 \n\t"

      "cbz r0,1f               \n\t"

      "mrs r0,psp              \n\t"

      "stmdb r0!,{r4-r11}      \n\t"

      "ldr r1,=current_task    \n\t"
      "cbz r1, done1            \n\t"
      "ldr r1,[r1]             \n\t"
      "str r0,[r1,#8]          \n\t" // change if stack placement is changed

      "1:                      \n\t"
      "push {lr}               \n\t"
      "bl next_task            \n\t"
      "pop {lr}                \n\t"

      "ldr r1,=current_task    \n\t"
      "ldr r1,[r1]             \n\t"
      "cbz r1, done2           \n\t"
      "ldr r0,[r1,#8]          \n\t"

      "ldmia r0!,{r4-r11}      \n\t"
      "msr psp,r0              \n\t"
      "cpsie i                 \n\t"

      "ldr lr, =0xFFFFFFFD     \n\t"
      "bx lr                   \n\t"

      "done1 :                   \n\t"
      "bl holy1                  \n\t"

      "done2 :                   \n\t"
      "bl holy2                  \n\t"
      
      :
      :
      : "memory");
}
