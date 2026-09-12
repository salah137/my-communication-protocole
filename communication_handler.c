#include "communication_lines/communication_line.h"
#include <stdint.h>

#define my_address 0x43
#define target_address 0x23

extern communication_line_t **lines;
extern uint8_t read_gpiob_level(uint8_t pin);
extern uint8_t accessible_address_count;
extern accessible_address_t **accessible_address;

void insert_accessible_address(uint8_t pin, uint8_t nodes_number,
                               uint8_t address) {
  uint8_t wrote = 0;

  for (uint8_t i = 0; i < accessible_address_count; i++) {
    // check if the address is already there;
    if (accessible_address[i]->address == address) {
      if (nodes_number < accessible_address[i]->nodes_number) {
        accessible_address[i]->nodes_number = nodes_number;
        accessible_address[i]->pin = pin;
      }
      wrote = 1;
      break;
    }
  }

  if (wrote == 0) {
    for (uint8_t i = 0; i < accessible_address_count; i++) {
      // check if there is any free spot in the array;
      if (read_gpiob_level(accessible_address[i]->pin) == 0) {
        accessible_address[i]->pin = pin;
        accessible_address[i]->nodes_number = nodes_number;
        accessible_address[i]->address = address;
        wrote = 1;
        break;
      }
    }
  }

  if (wrote == 0) {
    for (uint8_t i = 0; i < accessible_address_count; i++) {

      if (accessible_address[i]->address == 0 &&
          accessible_address[i]->pin == 0 &&
          accessible_address[i]->nodes_number == 0) {
        accessible_address[i]->pin = pin;
        accessible_address[i]->nodes_number = nodes_number;
        accessible_address[i]->address = address;
        wrote = 1;
        break;
      }
    }

    if (wrote == 0) {
      if (accessible_address_count < 15) {
        accessible_address[++accessible_address_count]->address = address;
        accessible_address[accessible_address_count]->pin = pin;
        accessible_address[accessible_address_count]->nodes_number =
            nodes_number;
      }
    }
  }

  if (wrote == 1) {
    for (uint8_t i = 0; i < accessible_address_count; i++) {
      if ((accessible_address[i]->address == address) &&
          (nodes_number != accessible_address[i]->nodes_number)) {
        accessible_address[i]->address = 0;
        accessible_address[i]->nodes_number = 0;
        accessible_address[i]->pin = 0;
      }
    }
  }
}

void Communication_handler(void *params) {
  communication_line_t *params_list = (communication_line_t *)params;
  communication_line_rx_param_t *rx_params = params_list->params_rx;
  communication_line_tx_param_t *tx_params = params_list->params_tx;

  if (rx_params->finished_reading_mode == 1) {
    switch (rx_params->mode) {
    case SEARCHING:
      break;

    case DATA_TRANSIT:
      break;

    case DATA_RECIVED:
      break;

    case TARGET_EXISTS:
      break;

    default:
      break;
    }
    rx_params->finished_reading_mode = 0;
  }

  if (rx_params->finished_reading_address == 1) {
    switch (rx_params->mode) {
    case SEARCHING:
      if (rx_params->address_buffer == my_address) {
        tx_params->address_buffer = my_address;
        tx_params->data_buffer = 0b00000000;
        tx_params->s_mode = TARGET_EXISTS;
        params_list->writing_func((void *)tx_params);
      } else {

        int8_t smallest_path = -1;
        // check if the target is already registred
        for (uint8_t i = 0; i < accessible_address_count; i++) {
          if (accessible_address[i]->address == my_address &&
              (read_gpiob_level(accessible_address[i]->pin) != 0) &&
              ((accessible_address[i]->nodes_number < smallest_path) ||
               (smallest_path == -1))) {
            smallest_path = accessible_address[i]->nodes_number;
          }
        }

        if (smallest_path == -1) {
          // broadcast the searching to other nodes
          for (uint8_t i = 0; i < 4; i++) {
            lines[i]->params_tx->s = SENDING_MODE;
            lines[i]->params_tx->s_mode = SEARCHING;
            lines[i]->params_tx->address_buffer = rx_params->address_buffer;

            lines[i]->writing_func((void *)lines[i]->params_tx);
          }
        } else {
          // broadcast back;
          lines[tx_params->pin - 1]->params_tx->s = SENDING_MODE;
          lines[tx_params->pin - 1]->params_tx->s_mode = SEARCHING;
          lines[tx_params->pin - 1]->params_tx->address_buffer =
              rx_params->address_buffer;
          lines[tx_params->pin - 1]->params_tx->data_buffer = smallest_path;
        }
      }
      break;

    case DATA_TRANSIT:

      break;

    case DATA_RECIVED:

      break;

    case TARGET_EXISTS:
      uint8_t wrote = 0;

      for (uint8_t i = 0; i < accessible_address_count; i++) {
        // check if the address is already there;
        if (accessible_address[i]->address == rx_params->address_buffer) {
          if (rx_params->data_buffer < accessible_address[i]->nodes_number) {
            accessible_address[i]->nodes_number = rx_params->data_buffer;
            accessible_address[i]->pin = rx_params->pin;
          }
          wrote = 1;
          break;
        }
      }

      if (wrote == 0) {
        for (uint8_t i = 0; i < accessible_address_count; i++) {
          // check if there is any free spot in the array;
          if (read_gpiob_level(accessible_address[i]->pin) == 0) {
            accessible_address[i]->pin = rx_params->pin;
            accessible_address[i]->nodes_number = rx_params->data_buffer;
            accessible_address[i]->address = rx_params->data_buffer;
            wrote = 1;
            break;
          }
        }
      }

      if (wrote == 0) {
        for (uint8_t i = 0; i < accessible_address_count; i++) {

          if (accessible_address[i]->address == 0 &&
              accessible_address[i]->pin == 0 &&
              accessible_address[i]->nodes_number == 0) {
            accessible_address[i]->pin = rx_params->pin;
            accessible_address[i]->nodes_number = rx_params->data_buffer;
            accessible_address[i]->address = rx_params->data_buffer;
            wrote = 1;
            break;
          }
        }
      }

      if (wrote == 1) {
        for (uint8_t i = 0; i < accessible_address_count; i++) {
          if ((accessible_address[i]->address == rx_params->address_buffer) &&
              (rx_params->data_buffer != accessible_address[i]->nodes_number)) {
            accessible_address[i]->address = 0;
            accessible_address[i]->nodes_number = 0;
            accessible_address[i]->pin = 0;
          }
        }
      }

      if (rx_params->address_buffer == target_address) {
        // assign this line and stop the loop
      } else {
        for (uint8_t i = 0; i < 4; i++) {
          lines[i]->params_tx->s_mode = TARGET_EXISTS;
          lines[i]->params_tx->s = SENDING_MODE;
          lines[i]->params_tx->address_buffer = rx_params->address_buffer;
          lines[i]->params_tx->data_buffer =
              accessible_address[i]->nodes_number + 1;
        }
      }

      break;

    default:
      break;
    }
    rx_params->finished_reading_address = 0;
  }

  if (rx_params->finished_reading_data == 1) {
    switch (rx_params->mode) {
    case SEARCHING:
      break;

    case DATA_TRANSIT:

      break;

    case DATA_RECIVED:

        // insert_accessible_address(rx_params->pin, uint8_t nodes_number, uint8_t address); holy shiiit
      if (rx_params->address_buffer == my_address) {
        // register the in; i don t know what i am sayin but you understand what
        // I meant;
      } else {
        for (uint8_t i = 0; i < accessible_address_count; i++) {
          if (accessible_address[i]->address == rx_params->address_buffer) {
            tx_params->s_mode = DATA_RECIVED;
            tx_params->address_buffer = rx_params->address_buffer;
            tx_params->data_buffer = rx_params->data_buffer;

            params_list->writing_func((void *) tx_params);
            break;
          }
        }
      }

      break;

    case TARGET_EXISTS:
      insert_accessible_address(rx_params->pin, rx_params->data_buffer,
                                rx_params->address_buffer);

      if (rx_params->address_buffer == target_address) {
        // assign this line and stop the loop
      } else {
        for (uint8_t i = 0; i < 4; i++) {
          lines[i]->params_tx->s_mode = TARGET_EXISTS;
          lines[i]->params_tx->s = SENDING_MODE;
          lines[i]->params_tx->address_buffer = rx_params->address_buffer;
          lines[i]->params_tx->data_buffer =
              accessible_address[i]->nodes_number + 1;
        }
      }

      break;

    default:
      break;
    }

    rx_params->finished_reading_data = 0;
  }
}
