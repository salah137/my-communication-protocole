#pragma once
#include "../tasks/tasks.h"
#include <stdint.h>

typedef enum {
    DATA_TRANSIT,
    TARGET_EXISTS,
    DATA_RECIVED,
    SEARCHING,
    WAITING
} modes_t;

typedef enum {
    RISING,
    FAILING,
    UNKOWN
} last_edge_t ;

typedef enum{
    READING_MODE,
    READING_ADDRESS,
    READING_BYTE
} reading_t;

typedef struct {
  uint8_t flag;
  void *body;
  uint8_t scratch_buffer;
  uint8_t mode_buffer;
  uint8_t data_buffer;
  uint8_t address_buffer;
  uint8_t pin;
  uint8_t i;
  int32_t last_falling_edge ;
  int32_t last_rising_edge;
  last_edge_t last_edge;
  reading_t r;
  modes_t mode;
} communication_line_param_t;

typedef struct {
  communication_line_param_t *param;
  thread_t *communication_thread;
  
} communication_line_t;

