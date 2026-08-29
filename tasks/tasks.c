#include "tasks.h"
#include "../dump_heap/dump_heap.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#define ALIGN 4

extern uint32_t _estack;
extern my_heap_t my_heap;

void thread_exit(void) {
  while (1) {
    __asm__ volatile("wfi");
  }
}

thread_t* current_thread;

thread_t * create_thread(char *name, void (*thread_function)(void *),
                  uint32_t stack_size, void *params,uint16_t id) {

  if (stack_size > 8192 || stack_size <= 0) {
    return 0;
  }

  stack_size = (stack_size + ALIGN - 1) & ~(ALIGN - 1);

    // define the head
    thread_t * thread = (thread_t *)allocate_dumb(&my_heap, sizeof(thread_t));
    if (thread == NULL) {
      return 0;
    }
    thread->name = name;
    thread->thread_function = thread_function;
    thread->id = id;
    thread->stack_top = &_estack - (stack_size / 4);
    thread->stack_end = thread->stack_top - (stack_size / 4);
    thread->stack_size = stack_size;
    thread->params = params;

    thread->next_task = NULL;

    uint32_t sp_addr = (uint32_t)thread->stack_top;

  uint32_t *sp = (uint32_t *)sp_addr;

  *(--sp) = (1U << 24);
  *(--sp) = (uint32_t)thread->thread_function;
  *(--sp) = (uint32_t)thread_exit; // LR
  *(--sp) = 0;                     // R12
  *(--sp) = 0;                     // R3
  *(--sp) = 0;                     // R2
  *(--sp) = 0;                     // R1
  *(--sp) = (uint32_t)params;      // R0: Passes 'params' into function

  for (int i = 0; i < 8; i++) {
    *(--sp) = 0;
  }

  return  thread;
}

