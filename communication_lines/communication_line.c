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

reading_t r = READING_MODE;

communication_line_t **lines;

last_edge_t last_edge = UNKOWN;

void Communication_Line1(void *params)
    __attribute((weak, alias("Communication_Line_Default")));
void Communication_Line2(void *params)
    __attribute((weak, alias("Communication_Line_Default")));
void Communication_Line3(void *params)
    __attribute((weak, alias("Communication_Line_Default")));
void Communication_Line4(void *params)
    __attribute((weak, alias("Communication_Line_Default")));

communication_line_t *create_line(uint8_t i) {
  communication_line_param_t *params =
      allocate_dumb(&my_heap, sizeof(communication_line_param_t));

  params->i = 0;
  params->last_falling_edge = -1;
  params->last_rising_edge = -1;

  params->mode = WAITING;
  params->r = READING_MODE;
  params->last_edge = UNKOWN;

  params->flag = 2;
  params->body = NULL;
  params->pin = i;

  thread_t *thread;

  switch (i) {
  case 1:
    thread =
        create_thread("line_1", Communication_Line1, 256, (void *)params, 0);
    break;

  case 2:
    thread =
        create_thread("line_2", Communication_Line2, 256, (void *)params, 0);
    break;

  case 3:
    thread =
        create_thread("line_3", Communication_Line3, 256, (void *)params, 0);
    break;

  case 4:
    thread =
        create_thread("line_4", Communication_Line4, 256, (void *)params, 0);
    break;

  default:
    return 0;
  }

  communication_line_t *line =
      allocate_dumb(&my_heap, sizeof(communication_line_t));
  line->communication_thread = thread;
  line->param = params;

  return line;
}

void Init_Communication_Lines(void) {
  lines = allocate_dumb(&my_heap, sizeof(communication_line_t *) * 15);

  communication_line_t *l;

  for (int i = 1; i <= 4; i++) {
    l = create_line(i);
    if (l == 0) {
      break;
      ;
    } else {
      lines[i - 1] = l;
    }
  }
}

int check_the_bite(communication_line_param_t *params_list) {
  switch (params_list->r) {
  case READING_MODE:
    if (params_list->i == 2) {
      params_list->mode_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->i = 0;

      // 11 : packet recived raise exeption or do something
      if ((params_list->mode_buffer) & (1) &&
          (params_list->mode_buffer) & (1 << 2)) {
        params_list->mode = DATA_RECIVED;
        params_list->r = READING_MODE;
      }
      // 01 : data transit
      else if ((params_list->mode_buffer) & (1) &&
               (((params_list->mode_buffer) & (1 << 2)) == 0)) {
        params_list->mode = DATA_TRANSIT;
        params_list->r = READING_ADDRESS;
      }
      // 10 :  target exists
      else if ((((params_list->mode_buffer) & (1)) == 0) &&
               (params_list->mode_buffer) & (1 << 2)) {
        params_list->mode = TARGET_EXISTS;
        params_list->r = READING_ADDRESS;
      }
      // 00 : searching for target
      else if ((((params_list->mode_buffer) & (1)) == 0) &&
               ((params_list->mode_buffer) & (1 << 2)) == 0) {
        params_list->mode = SEARCHING;
        params_list->r = READING_ADDRESS;
      }
    }
    break;

  case READING_ADDRESS:
    if (params_list->i == 8) {
      params_list->address_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->i = 0;

      switch (params_list->mode) {
      case DATA_TRANSIT:
        params_list->r = READING_BYTE;
        break;

      case SEARCHING:
        params_list->r = READING_MODE;
        break;

      case TARGET_EXISTS:
        params_list->r = READING_MODE;
        break;

      default:
        params_list->r = READING_MODE;
        break;
      }
    }
    break;
  case READING_BYTE:
    if (params_list->i == 8) {
      params_list->data_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->i = 0;

      params_list->r = READING_MODE;
    }
    break;
  }

  if (params_list->i == 0) {
    return 0;
  }

  return 1;
}

// coock this , not finished at all
void Communication_Line_Default(void *params) {
  communication_line_param_t *params_list =
      (communication_line_param_t *)params;

  if (params_list->flag == 0) {
    // read the data
  } else if (params_list->flag == 1) {
    if (read_gpiob_level(params_list->pin) == 0) {
      // falling edge
      if (params_list->last_rising_edge != -1) {
        params_list->last_falling_edge = timer_ticks;
        switch (last_edge) {
        case RISING:
          uint8_t bits_count = timer_ticks - params_list->last_rising_edge;

          for (uint8_t i = 0; i < bits_count; i++) {
            params_list->scratch_buffer =
                (params_list->scratch_buffer << 1) | (1);

            if (check_the_bite(params_list) == 0) {
              params_list->last_edge = UNKOWN;
              break;
            } else {
              params_list->i++;
              params_list->last_edge = FAILING;
            }
          }
          break;

        case FAILING:
          // impossible
          break;

        case UNKOWN:
          if (params_list->last_falling_edge == -1) {
            params_list->last_falling_edge = timer_ticks;
            
          } else {
            /* 
            uint8_t bits_count =
                (timer_ticks - params_list->last_falling_edge) - 1;
            for (uint8_t i = 0; i < bits_count; i++) {
              params_list->scratch_buffer =
                  (params_list->scratch_buffer << 1) & ~(1);

              if (check_the_bite(params_list) == 0) {
                params_list->last_edge = UNKOWN;
                break;
              } else {
                params_list->i++;
                params_list->last_edge = FAILING;
              }
            }
            */
          }
          break;
        }

      } else if (params_list->last_rising_edge == -1) {
      }
    }
    else if (read_gpiob_level(params_list->pin) == 1) {
      // rising_edge
      if (params_list->last_falling_edge != -1) {

        switch (last_edge) {

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
              break;
            } else {
              params_list->i++;
              params_list->last_edge = RISING;
            }
          }
          break;

        case UNKOWN:

          if (params_list->last_falling_edge == -1) {
            // quite impossible
          } else {
            uint8_t bits_count =
                (timer_ticks - params_list->last_falling_edge) - 1;
            for (uint8_t i = 0; i < bits_count; i++) {
              params_list->scratch_buffer =
                  (params_list->scratch_buffer << 1) & ~(1);

              if (check_the_bite(params_list) == 0) {
                params_list->last_edge = UNKOWN;
                break;
              } else {
                params_list->i++;
                params_list->last_edge = RISING;
              }
              
            }
          }
          break;
        }

        params_list->last_rising_edge = timer_ticks;
      }
      if (params_list->last_falling_edge == -1) {
      }
    }

  } else {
    // return the tha  MSP
  }

  __asm__ volatile("svc #0");

  return;
}

// 00000000;
