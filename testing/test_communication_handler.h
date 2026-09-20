#ifndef TEST_COMMUNICATION_LINE_H
#define TEST_COMMUNICATION_LINE_H

#include <stdint.h>

// Frame "mode" field — what kind of packet this is.
typedef enum {
  SEARCHING = 0,
  DATA_TRANSIT,
  TARGET_EXISTS,
  DATA_RECIVED
} modes_t;

// Line direction state — what a given line's TX side should be doing.
typedef enum {
  IDLE_MODE = 0,
  RECEIVING_MODE,
  SENDING_MODE
} line_state_t;

// One entry in the routing table: "address is reachable via pin, at
// nodes_number hops".
typedef struct {
  uint8_t address;
  uint8_t pin;
  uint8_t nodes_number;
} accessible_address_t;

// RX-side state for one physical line (populated by the bit-banged
// GPIO edge-timing decoder as a frame comes in).
typedef struct {
  uint8_t pin;
  modes_t mode;
  uint8_t address_buffer;
  uint8_t address2_buffer;
  uint8_t data_buffer;
  uint8_t finished_reading_address2;
  uint8_t finished_reading_data;
} communication_line_rx_param_t;

// TX-side state for one physical line (populated by the handler,
// consumed by writing_func to actually drive the GPIO).
typedef struct {
  uint8_t pin;
  modes_t s_mode;
  line_state_t s;
  uint8_t address_buffer;
  uint8_t address2_buffer;
  uint8_t data_buffer;
} communication_line_tx_param_t;

typedef struct communication_line_t {
  communication_line_rx_param_t *params_rx;
  communication_line_tx_param_t *params_tx;
  void (*writing_func)(void *params);
} communication_line_t;

void insert_accessible_address(uint8_t pin, uint8_t nodes_number,
                               uint8_t address);
int8_t search_for_node(uint8_t address);
void Communication_handler(void *params);

#endif // COMMUNICATION_LINE_H