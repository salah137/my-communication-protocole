#include "test_communication_handler.h"
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
        nodes_number = accessible_address[i]->nodes_number;
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
        accessible_address[accessible_address_count]->address = address;
        accessible_address[accessible_address_count]->pin = pin;
        accessible_address[accessible_address_count++]->nodes_number =
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

int8_t search_for_node(uint8_t address) {
  int8_t smallest_path = -1;

  for (uint8_t i = 0; i < accessible_address_count; i++) {
    if (accessible_address[i]->address == address) {
        if (smallest_path == -1 ||
            accessible_address[i]->nodes_number < accessible_address[smallest_path]->nodes_number) {
          smallest_path = i;
        }    
    }
  }

  return smallest_path;
}

void Communication_handler(void *params) {
  communication_line_t *params_list = (communication_line_t *)params;
  communication_line_rx_param_t *rx_params = params_list->params_rx;
  communication_line_tx_param_t *tx_params = params_list->params_tx;

  if (rx_params->finished_reading_address2 == 1) {
    if (rx_params->mode == DATA_RECIVED) {
      if (rx_params->address2_buffer == my_address) {
        // do something; TODO: make the firemware knows about the handshake
      } else {
        int8_t smallest_path = search_for_node(rx_params->address2_buffer);
        if (smallest_path != -1) {
          lines[accessible_address[smallest_path]->pin - 1]
              ->params_tx->address_buffer = rx_params->address_buffer;
          lines[accessible_address[smallest_path]->pin - 1]
              ->params_tx->address2_buffer = rx_params->address2_buffer;
          lines[accessible_address[smallest_path]->pin - 1]->params_tx->s =
              SENDING_MODE;
          lines[accessible_address[smallest_path]->pin - 1]->params_tx->s_mode =
              DATA_RECIVED;

          lines[accessible_address[smallest_path]->pin - 1]->writing_func(
              (void *)lines[accessible_address[smallest_path]->pin - 1]
                  ->params_tx);
        } else {
          // do something; TODO: raise an exeption or something;
        }
      }
    }
    rx_params->finished_reading_address2 = 0;
  }

  if (rx_params->finished_reading_data == 1) {
    switch (rx_params->mode) {
    case SEARCHING:

      insert_accessible_address(rx_params->pin, rx_params->data_buffer,
                                rx_params->address2_buffer);

      if (rx_params->address2_buffer == my_address) {
        tx_params->address2_buffer = my_address;
        tx_params->address_buffer = rx_params->address_buffer;
        tx_params->data_buffer = 0b00000000;
        tx_params->s_mode = TARGET_EXISTS;
        params_list->writing_func((void *)tx_params);
      } else {
        int8_t searching_node = search_for_node(rx_params->address2_buffer);
        int8_t sender_node = search_for_node(rx_params->address_buffer);

        if (searching_node != -1 &&
            read_gpiob_level(accessible_address[searching_node]->pin) != 0) {
          if (sender_node != -1) {
            lines[accessible_address[sender_node]->pin - 1]
                ->params_tx->address_buffer = rx_params->address_buffer;

            lines[accessible_address[sender_node]->pin - 1]
                ->params_tx->address2_buffer = rx_params->address2_buffer;

            lines[accessible_address[sender_node]->pin - 1]
                ->params_tx->data_buffer =
                accessible_address[searching_node]->nodes_number;

            lines[accessible_address[sender_node]->pin - 1]->params_tx->s_mode =
                TARGET_EXISTS;

            lines[accessible_address[sender_node]->pin - 1]->params_tx->s =
                SENDING_MODE;

            lines[accessible_address[sender_node]->pin - 1]->writing_func(
                (void *)lines[accessible_address[sender_node]->pin - 1]
                    ->params_tx);

          } else {
            tx_params->address_buffer = rx_params->address_buffer;
            tx_params->address2_buffer = rx_params->address2_buffer;
            tx_params->data_buffer =
                accessible_address[searching_node]->nodes_number;
            tx_params->s_mode = TARGET_EXISTS;
            tx_params->s = SENDING_MODE;

            params_list->writing_func((void *)tx_params);
          }
        } else {
          for (uint8_t i = 0; i < 4; i++) {
            lines[i]->params_tx->address_buffer = rx_params->address_buffer;
            lines[i]->params_tx->address2_buffer = rx_params->address2_buffer;
            lines[i]->params_tx->data_buffer = rx_params->data_buffer + 1;
            lines[i]->params_tx->s_mode = SEARCHING;
            lines[i]->params_tx->s = SENDING_MODE;

            lines[i]->writing_func((void *)lines[i]->params_tx);
          }
        }
      }
      break;

    case DATA_TRANSIT:
      if (rx_params->address2_buffer == my_address) {
        tx_params->address_buffer = rx_params->address2_buffer;
        tx_params->address2_buffer = my_address;

        tx_params->s_mode = DATA_RECIVED;
        tx_params->s = SENDING_MODE;

        params_list->writing_func((void *)tx_params);
        // TODO : fire the software interrupt
      } else {
        // PASS THE DATA TO THE NEXT NODE
        int8_t searching_for_node = search_for_node(rx_params->address2_buffer);
        if (searching_for_node != -1) {
          lines[accessible_address[searching_for_node]->pin - 1]
              ->params_tx->address_buffer = rx_params->address_buffer;
          lines[accessible_address[searching_for_node]->pin - 1]
              ->params_tx->address2_buffer = rx_params->address2_buffer;
          lines[accessible_address[searching_for_node]->pin - 1]
              ->params_tx->data_buffer = rx_params->data_buffer;
          lines[accessible_address[searching_for_node]->pin - 1]
              ->params_tx->s_mode = DATA_TRANSIT;
          lines[accessible_address[searching_for_node]->pin - 1]->params_tx->s =
              SENDING_MODE;

          lines[accessible_address[searching_for_node]->pin - 1]->writing_func(
              (void *)lines[accessible_address[searching_for_node]->pin - 1]
                  ->params_tx);
        } else {
          // TODO : RAISE an EXEPTION
        }
      }

      break;

    case TARGET_EXISTS:
      insert_accessible_address(rx_params->pin, rx_params->data_buffer,
                                rx_params->address2_buffer);

      if (rx_params->address_buffer == target_address) {
        // TODO : assign this line and stop the loop

      } else {
        int8_t target_node = search_for_node(rx_params->address2_buffer);
        if (target_node != -1) {
          lines[accessible_address[target_node]->pin - 1]
              ->params_tx->address_buffer = rx_params->address_buffer;
          lines[accessible_address[target_node]->pin - 1]
              ->params_tx->address2_buffer = rx_params->address2_buffer;
          lines[accessible_address[target_node]->pin - 1]
              ->params_tx->data_buffer = rx_params->data_buffer + 1;
          lines[accessible_address[target_node]->pin - 1]->params_tx->s_mode =
              TARGET_EXISTS;

          lines[accessible_address[target_node]->pin - 1]->params_tx->s =
              SENDING_MODE;
          lines[accessible_address[target_node]->pin - 1]->writing_func(
              (void *)lines[accessible_address[target_node]->pin - 1]
                  ->params_tx);
        }  else {
            //TODO: raise exeption
        }
      } 

      break;

    default:
      break;
    }

    rx_params->finished_reading_data = 0;
  }
}
