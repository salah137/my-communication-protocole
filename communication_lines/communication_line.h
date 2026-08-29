#pragma once
#include "../tasks/tasks.h"

typedef struct {
  uint8_t flag;
  void *body;
} communication_line_param_t;

typedef struct {
  communication_line_param_t *param;
  thread_t *communication_thread;
} communication_line_t;
