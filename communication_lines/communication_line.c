#include "communication_line.h"
#include "../dump_heap/dump_heap.h"
#include "../tasks/tasks.h"
#include <stdint.h>

extern void *allocate_dumb(my_heap_t *heap, size_t size);
extern thread_t *create_thread(char *name, void (*thread_function)(void *),
                               uint32_t stack_size, void *params, uint16_t id);
extern my_heap_t my_heap;
extern uint8_t read_gpiob_level(uint8_t pin);
extern volatile uint32_t timer_ticks;
extern void reset_pin_a(uint8_t pin);
extern void set_pin_a(uint8_t pin);

communication_line_t **lines;

void Communication_Line1_Read(void *params)
    __attribute((weak, alias("Communication_Line_Default_Read")));
void Communication_Line2_Read(void *params)
    __attribute((weak, alias("Communication_Line_Default_Read")));
void Communication_Line3_Read(void *params)
    __attribute((weak, alias("Communication_Line_Default_Read")));
void Communication_Line4_Read(void *params)
    __attribute((weak, alias("Communication_Line_Default_Read")));

void Communication_Line1_Write(void *params)
    __attribute((weak, alias("Communication_Line_Default_Write")));
void Communication_Line2_Write(void *params)
    __attribute((weak, alias("Communication_Line_Default_Write")));
void Communication_Line3_Write(void *params)
    __attribute((weak, alias("Communication_Line_Default_Write")));
void Communication_Line4_Write(void *params)
    __attribute((weak, alias("Communication_Line_Default_Write")));

communication_line_t *create_line(uint8_t i) {
  communication_line_rx_param_t *params_rx =
      allocate_dumb(&my_heap, sizeof(communication_line_rx_param_t));

  communication_line_tx_param_t *params_tx =
      allocate_dumb(&my_heap, sizeof(communication_line_tx_param_t));

  params_rx->recieved_bits = 0;
  params_rx->last_falling_edge = -1;
  params_rx->last_rising_edge = -1;

  params_rx->mode = WAITING;
  params_rx->r = READING_MODE;
  params_rx->last_edge = UNKOWN;

  params_rx->pin = i;

  thread_t *thread;

  switch (i) {
  case 1:
    thread = create_thread("line_1", Communication_Line1_Read, 256,
                           (void *)params_rx, 0);
    break;

  case 2:
    thread = create_thread("line_2", Communication_Line2_Read, 256,
                           (void *)params_rx, 0);
    break;

  case 3:
    thread = create_thread("line_3", Communication_Line3_Read, 256,
                           (void *)params_rx, 0);
    break;

  case 4:
    thread = create_thread("line_4", Communication_Line4_Read, 256,
                           (void *)params_rx, 0);
    break;

  default:
    return 0;
  }

  communication_line_t *line =
      allocate_dumb(&my_heap, sizeof(communication_line_t));

  switch (i) {
  case 1:
    line->writing_func = Communication_Line1_Write;
    break;

  case 2:
    line->writing_func = Communication_Line2_Write;
    break;

  case 3:
    line->writing_func = Communication_Line3_Write;
    break;

  case 4:
    line->writing_func = Communication_Line4_Write;
    break;
  }

  line->communication_thread = thread;
  line->params_rx = params_rx;
  line->params_tx = params_tx;

  return line;
}

void Init_Communication_Lines(void) {
  lines = allocate_dumb(&my_heap, sizeof(communication_line_t *) * 4);

  communication_line_t *l;

  for (int i = 1; i <= 4; i++) {
    l = create_line(i);
    if (l == 0) {
      break;

    } else {
      lines[i - 1] = l;
    }
  }
}

int check_the_bite(communication_line_rx_param_t *params_list) {
    if(params_list->recieved_bits == 0){
        return 1;
    }
  switch (params_list->r) {
  case READING_MODE:
    if (params_list->recieved_bits == 1) {
      params_list->mode_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->recieved_bits = 0;

      if ((((params_list->mode_buffer) & (1 << 0)) != 0) &&
          (((params_list->mode_buffer) & (1 << 1)) != 0)) {
        params_list->mode = DATA_RECIVED;
        params_list->r = READING_MODE;
      } else if ((((params_list->mode_buffer) & (1 << 0)) != 0) &&
                 (((params_list->mode_buffer) & (1 << 1)) == 0)) {
        params_list->mode = DATA_TRANSIT;
        params_list->r = READING_ADDRESS;
      } else if ((((params_list->mode_buffer) & (1 << 0)) == 0) &&
                 (((params_list->mode_buffer) & (1 << 1)) != 0)) {
        params_list->mode = TARGET_EXISTS;
        params_list->r = READING_ADDRESS;
      } else if ((((params_list->mode_buffer) & (1 << 0)) == 0) &&
                 ((params_list->mode_buffer) & (1 << 1)) == 0) {
        params_list->mode = SEARCHING;
        params_list->r = READING_ADDRESS;
      }
    }
    break;
  case READING_ADDRESS:
    if (params_list->recieved_bits == 7) {
      params_list->address_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->recieved_bits = 0;

      switch (params_list->mode) {
      case DATA_TRANSIT:
        params_list->r = READING_BYTE;
        break;

      case SEARCHING:
      case TARGET_EXISTS:
      default:
        params_list->r = READING_MODE;
        break;
      }
    }
    break;
  case READING_BYTE:
    if (params_list->recieved_bits == 7) {
      params_list->data_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->recieved_bits = 0;

      params_list->r = READING_MODE;
    }
    break;
  }

  if (params_list->recieved_bits == 0) {
    return 0;
  }

  return 1;
}

void Communication_Line_Default_Read(void *params) {
  communication_line_rx_param_t *params_list =
      (communication_line_rx_param_t *)params;

  // falling edge

  if (read_gpiob_level(params_list->pin) == 0) {

    switch (params_list->last_edge) {
    case RISING:
      uint8_t bits_count = timer_ticks - params_list->last_rising_edge;

      for (uint8_t i = 0; i < bits_count; i++) {
        params_list->scratch_buffer =
            (params_list->scratch_buffer << 1) | (1);

          if (check_the_bite(params_list) == 0) {
            params_list->last_edge = UNKOWN;
            params_list->scratch_buffer = 0;
            params_list->last_falling_edge = -1;
            break;
          } else {
            params_list->recieved_bits++;
            params_list->last_edge = FAILING;
            params_list->last_falling_edge = timer_ticks;
          }
      }
      break;

    case FAILING:
      // impossible
      break;

    case UNKOWN:
      if (params_list->last_falling_edge == -1) {
        params_list->last_falling_edge = timer_ticks;
      }
      break;
    }

    // rising_edge
  } else if (read_gpiob_level(params_list->pin) == 1) {
    switch (params_list->last_edge) {

    case RISING:
      // impossible
      break;

    case FAILING:
      uint8_t bits_count = timer_ticks - params_list->last_falling_edge;

      for (uint8_t i = 0; i < bits_count; i++) {
          params_list->scratch_buffer =
              (params_list->scratch_buffer << 1) & ~(1);

          if (check_the_bite(params_list) == 0) {
            params_list->last_edge = UNKOWN;
            params_list->scratch_buffer = 0;
            params_list->last_falling_edge = -1;
            break;
          } else {
            params_list->recieved_bits++;
            params_list->last_edge = RISING;
          }

      }
      break;

    case UNKOWN:

      if (params_list->last_falling_edge != -1) {
        uint8_t bits_count = (timer_ticks - params_list->last_falling_edge) - 1;

        if (bits_count == 0) {
          params_list->last_edge = RISING;
        }
        for (uint8_t i = 0; i < bits_count; i++) {
          params_list->scratch_buffer =
              (params_list->scratch_buffer << 1) & ~(1);
            if (check_the_bite(params_list) != 0) {
              params_list->last_edge = UNKOWN;
              params_list->last_falling_edge = -1;
              break;
            } else {
              params_list->recieved_bits++;
              params_list->last_edge = RISING;
              params_list->last_rising_edge = timer_ticks;
            }
        }
      }
      break;
    }

    params_list->last_rising_edge = timer_ticks;
  }

  return;
}

void Communication_Line_Default_Write(void *params) {
  communication_line_tx_param_t *param_list =
      (communication_line_tx_param_t *)params;

  switch (param_list->s) {

  case SENDING_MODE:
    while (1) {
      if (timer_ticks != param_list->last_write_tick) {
        switch (param_list->sent_bits) {
        case 0:
          reset_pin_a(param_list->pin);
          param_list->sent_bits++;
          break;

        case 1:
          switch (param_list->s_mode) {
          case SEARCHING:
            reset_pin_a(param_list->pin);
            break;

          case DATA_TRANSIT:
            set_pin_a(param_list->pin);
            break;

          case DATA_RECIVED:
            set_pin_a(param_list->pin);
            break;

          case TARGET_EXISTS:
            reset_pin_a(param_list->pin);
            break;

          default:
            break;
          }
          param_list->sent_bits++;
          break;

        case 2:
          switch (param_list->s_mode) {
          case SEARCHING:
            reset_pin_a(param_list->pin);
            break;

          case DATA_TRANSIT:
            reset_pin_a(param_list->pin);
            break;

          case DATA_RECIVED:
            set_pin_a(param_list->pin);
            break;

          case TARGET_EXISTS:
            set_pin_a(param_list->pin);
            break;

          default:
            break;
          }
          param_list->sent_bits++;
          break;

        case 3:
          switch (param_list->s_mode) {
          case SEARCHING:
            param_list->s = SENDING_ADDRESS;
            break;

          case DATA_TRANSIT:
            param_list->s = SENDING_ADDRESS;
            reset_pin_a(param_list->pin);
            break;

          case DATA_RECIVED:
            param_list->s = SENDING_MODE;
            set_pin_a(param_list->pin);
            break;

          case TARGET_EXISTS:
            param_list->s = SENDING_ADDRESS;

            set_pin_a(param_list->pin);
            break;

          default:
            break;
          }
          param_list->sent_bits = 0;
          break;

        default:
          break;
        }

        param_list->last_write_tick = timer_ticks;

        if (param_list->sent_bits == 0) {
          break;
        }
      }
    }

    while (param_list->sent_bits <= 1) {
      if (timer_ticks != param_list->last_write_tick) {          
        set_pin_a(param_list->pin);
        param_list->sent_bits++;
        param_list->last_write_tick = timer_ticks;
      }
    }
    param_list->sent_bits = 0;
    if (param_list->s == SENDING_ADDRESS) {
      lines[param_list->pin - 1]->writing_func((void *)param_list);
    }
    break;

  case SENDING_ADDRESS:

      while (param_list->sent_bits <= 0) {
        if (timer_ticks != param_list->last_write_tick) {            
          reset_pin_a(param_list->pin);
          param_list->sent_bits++;
          param_list->last_write_tick = timer_ticks;
        }
      }

    while (1) {
      if (param_list->last_write_tick != timer_ticks) {
        if (param_list->sent_bits <= 8) {
          uint8_t bit_to_write = (((param_list->address_buffer) &
                                   (1 << param_list->sent_bits)) == 0)
                                     ? 0
                                     : 1;

          if (bit_to_write == 1) {
            set_pin_a(param_list->pin);
          } else if (bit_to_write == 0) {
            reset_pin_a(param_list->pin);
          }
          if (param_list->sent_bits == 8) {
              while (1) {
                if (param_list->last_write_tick != timer_ticks) {
                  reset_pin_a(param_list->pin);
                  param_list->last_write_tick = timer_ticks;
                  break;
                }
              }

            switch (param_list->s_mode) {
            case SEARCHING:
              param_list->s = SENDING_MODE;
              break;
            case DATA_TRANSIT:
              param_list->s = SENDING_BYTE;

              break;
            case TARGET_EXISTS:
              param_list->s = SENDING_MODE;
              break;
            default:
              break;
            }
            break;
          }
          param_list->sent_bits++;
          param_list->last_write_tick = timer_ticks;
        }
      }
    }
    
    param_list->sent_bits = 0;
    if (param_list->s == SENDING_BYTE) {
      while (param_list->sent_bits <= 1) {
        if (timer_ticks != param_list->last_write_tick) {
          set_pin_a(param_list->pin);
          param_list->sent_bits++;
          param_list->last_write_tick = timer_ticks;
        }
      }
      param_list->sent_bits = 0;
      
      lines[param_list->pin - 1]->writing_func((void *)param_list);
    }
 
    break;
  case SENDING_BYTE:
      while (param_list->sent_bits <= 0) {
        if (timer_ticks != param_list->last_write_tick) {            
          reset_pin_a(param_list->pin);
          param_list->sent_bits++;
          param_list->last_write_tick = timer_ticks;
        }
      }

    while (1) {
      if (param_list->last_write_tick != timer_ticks) {
        if (param_list->sent_bits <= 8) {
          uint8_t bit_to_write =
              (((param_list->data_buffer) & (1 << param_list->sent_bits)) == 0)
                  ? 0
                  : 1;

          if (bit_to_write == 1) {
            set_pin_a(param_list->pin);
          } else if (bit_to_write == 0) {
            reset_pin_a(param_list->pin);
          }
          if (param_list->sent_bits == 8) {
            param_list->s = SENDING_MODE;
            reset_pin_a(param_list->pin);
            param_list->last_write_tick = timer_ticks;

            param_list->sent_bits = 0;
            while (param_list->sent_bits <= 1) {
              if (timer_ticks != param_list->last_write_tick) {
                set_pin_a(param_list->pin);
                param_list->sent_bits++;
                param_list->last_write_tick = timer_ticks;
              }
            }
            param_list->sent_bits = 0;
            
            break;
          }
          param_list->sent_bits++;
          param_list->last_write_tick = timer_ticks;
        }
      }
    }
    break;
  }
}
