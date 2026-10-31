#include "tasks.h"
#include "../dump_heap/dump_heap.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#define ALIGN 4
#define MAIN_STACK_SIZE 1024

extern uint32_t _estack;
extern my_heap_t my_heap;
static int last_id = 0;

thread_t *head = (thread_t *)0;
thread_t *current_task = (thread_t *)0;
thread_t *running_thread;
static uint32_t *next_top;

void thread_exit(void) {

}

void build_frame(thread_t* new_task,void* params){
    uint32_t sp_addr = (uint32_t)new_task->stack_top;

    uint32_t *sp = (uint32_t *)sp_addr;

    *(--sp) = (1U << 24);
    *(--sp) = (uint32_t)new_task->thread_function;
    *(--sp) = (uint32_t)thread_exit; // LR
    *(--sp) = 0;                     // R12
    *(--sp) = 0;                     // R3
    *(--sp) = 0;                     // R2
    *(--sp) = 0;                     // R1
    *(--sp) = (uint32_t)params;      // R0: Passes 'params' into function

    for (int i = 0; i < 8; i++) {
      *(--sp) = 0;
    }

    new_task->stack_top = sp;
}
/*
thread_t * create_thread_unlinked(char *name, void (*thread_function)(void *),
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
    thread->stack_top = &_estack -  MAIN_STACK_SIZE ;
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
*/

thread_t *create_thread_linked(char *name, void (*thread_function)(void *),
                               uint32_t stack_size, void *params,
                               uint8_t linked) {

  if (linked == 0 && head == 0)
    return (thread_t *)-1;

  if (stack_size > 8192 || stack_size <= 0) {
    return (thread_t *)-1;
  }

  stack_size = (stack_size + ALIGN - 1) & ~(ALIGN - 1);

  if (head == 0) {
    // define the head
    head = (thread_t *)allocate_dumb(&my_heap, sizeof(thread_t));
    if (head == NULL) {
      return (thread_t *)-1;
    }
    head->name = name;
    head->thread_function = thread_function;
    head->id = 0;
    head->stack_top = &_estack - MAIN_STACK_SIZE/4;
    head->stack_end = head->stack_top - (stack_size / 4);
    head->stack_size = stack_size;
    head->params = params;
    next_top = head->stack_top - (stack_size / 4);
    head->next_task = NULL;

    build_frame(head, params);
    
    current_task = head;
  } else {

    thread_t *new_task = (thread_t *)allocate_dumb(&my_heap, sizeof(thread_t));
    if (new_task == NULL) {
      return (thread_t *)-1;
    }
    new_task->name = name;
    new_task->thread_function = thread_function;

    new_task->stack_top = next_top;
    new_task->stack_end = new_task->stack_top - (stack_size / 4);
    next_top = new_task->stack_top - (stack_size / 4);
    new_task->stack_size = stack_size;
    new_task->params = params;

    new_task->next_task = NULL;

    build_frame(new_task, params);
    

    if (linked == 1) {
      new_task->id = current_task->id + 1;
      current_task->next_task = new_task;

      current_task = new_task;

      last_id = new_task->id;
      return current_task;
    } else {
      return new_task;
    }
  }

  // prepare stack

  return 0;
}

void start_routines() { current_task = head; }

void next_task() {
  if (current_task->id != last_id) {
    current_task = current_task->next_task;
  } else {
    start_routines();
  }
  return;
}
//0x20001bd0