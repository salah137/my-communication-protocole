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

typedef enum { RISING, FAILING, UNKOWN } last_edge_t;

typedef enum {
  READING_MODE,
  READING_ADDRESS,
  READING_ADDRESS2,
  READING_BYTE
} reading_t;

typedef enum {
  SENDING_MODE,
  SENDING_ADDRESS,
  SENDING_ADDRESS2,
  SENDING_BYTE
} sending_t;

typedef struct {
  uint8_t scratch_buffer;
  uint8_t mode_buffer;
  uint8_t finished_reading_mode;

  uint8_t data_buffer;
  uint8_t finished_reading_data;

  uint8_t address_buffer;
  uint8_t finished_reading_address;

  uint8_t address2_buffer;
  uint8_t finished_reading_address2;

  uint8_t pin;
  uint8_t recieved_bits;
  uint8_t sent_bits;
  int32_t last_falling_edge;
  int32_t last_rising_edge;
  last_edge_t last_edge;
  reading_t r;
  modes_t mode;

  void (*recived_data_intr)(void *);
  void *recived_data_intr_params;

  void (*data_handshake_intr)(void *);
  void *data_handshake_intr_params;

  void (*existing_target_intr)(void *);
  void *existing_target_intr_params;

} communication_line_rx_param_t;

typedef struct {
  uint8_t data_buffer;
  uint8_t address_buffer;
  uint8_t address2_buffer;
  uint8_t pin;
  uint8_t sent_bits;
  int32_t last_write_tick;
  sending_t s;
  modes_t s_mode;
} communication_line_tx_param_t;

typedef struct {
  uint8_t address;
  uint8_t nodes_number;
  uint8_t pin;
} accessible_address_t;

typedef struct {
  communication_line_rx_param_t *params_rx;
  communication_line_tx_param_t *params_tx;
  thread_t *communication_thread;
  void (*writing_func)(void *);

} communication_line_t;

typedef struct {
  communication_line_t *line;
} handler_params_t;

typedef struct {
  uint8_t buffer;
  uint8_t line;
  uint8_t address;
} recived_data_t;

typedef struct {
  uint8_t address;
  uint8_t line;
} data_handshake_t;

typedef struct {
  uint8_t address;
  uint8_t line;
  uint8_t hoops_number;
} target_exists_t;

typedef struct {
  uint8_t machine_address;
  uint8_t target_address;
  data_handshake_t *data_handshake;
  recived_data_t *recived_data;
  target_exists_t *existing_target;

} machine_state_t;
