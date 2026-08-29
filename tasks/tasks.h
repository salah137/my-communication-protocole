#pragma once

#include "stdint.h"

typedef struct thread {
  char *name;
  uint32_t id;
  uint32_t *stack_top;
  uint32_t *stack_end;
  uint32_t stack_size;
  void *params;
  void (*thread_function)(void *);
  struct thread *next_task;
} thread_t;
