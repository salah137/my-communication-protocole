#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef enum {
  DATA_TRANSIT,
  TARGET_EXISTS,
  DATA_RECIVED,
  SEARCHING,
  WAITING
} modes_t;

typedef enum { RISING, FAILING, UNKOWN } last_edge_t;

typedef enum { READING_MODE, READING_ADDRESS, READING_BYTE } reading_t;

typedef enum { SENDING_MODE, SENDING_ADDRESS, SENDING_BYTE } sending_t;

typedef struct {
  uint8_t scratch_buffer;
  uint8_t mode_buffer;
  uint8_t data_buffer;
  uint8_t address_buffer;
  uint8_t pin;
  uint8_t recieved_bits;
  uint8_t sent_bits;
  uint8_t s_data;
  int32_t last_falling_edge;
  int32_t last_rising_edge;
  last_edge_t last_edge;
  reading_t r;
  modes_t mode;
} communication_line_rx_param_t;

typedef struct {
  uint8_t data_buffer;
  uint8_t address_buffer;
  uint8_t pin;
  uint8_t sent_bits;
  int32_t last_write_tick;
  sending_t s;
  modes_t s_mode;
} communication_line_tx_param_t;

typedef struct {
  communication_line_rx_param_t *params_rx;
  communication_line_tx_param_t *params_tx;
  // thread_t *communication_thread;
  void (*writing_func)(void *);
} communication_line_t;

volatile uint32_t timer_ticks = 0;
uint8_t simulated_gpio_stream[100];
uint32_t max_stream_ticks = 0;
communication_line_tx_param_t test_param = {0};
communication_line_t instance = {0};
uint8_t read_gpiob_level(uint8_t pin) {
  if (timer_ticks < max_stream_ticks) {
    return simulated_gpio_stream[timer_ticks];
  }
  return 1; // Idle HIGH
}

int p = 0;

void reset_pin_a(uint8_t pin) { simulated_gpio_stream[p++] = 0; }

void set_pin_a(uint8_t pin) { simulated_gpio_stream[p++] = 1; }

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
          printf("holola \n");
          
        set_pin_a(param_list->pin);
        param_list->sent_bits++;
        param_list->last_write_tick = timer_ticks;
      }
    }
    param_list->sent_bits = 0;
    if (param_list->s == SENDING_ADDRESS) {
      instance.writing_func((void *)param_list);
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
                  printf("hola reset\n");
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
      
      instance.writing_func((void *)param_list);
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

// TODO : test the writ11010110ing, not that one in the wall!
void testing_write(void) {
  instance.writing_func = Communication_Line_Default_Write;
  test_param.s_mode = DATA_TRANSIT;
  test_param.address_buffer = 0b11101110;
  test_param.data_buffer = 0b11010110;

  instance.writing_func(&test_param);
}

static pthread_t ticker_thread;
static volatile int ticker_running = 1;

void *tick_isr(void *arg) {
  while (ticker_running) {
    usleep(1000); // 1ms "tick" - tune to whatever your real SysTick period is
    timer_ticks++;
    //  printf("%d \n",timer_ticks);
  }
  return NULL;
}

void test_array() {
  int size = sizeof(simulated_gpio_stream);
  for (int i = 0; i < size; i++) {
    printf("%d", simulated_gpio_stream[i]);
  }
}

int main(void) {
  pthread_create(&ticker_thread, NULL, tick_isr, NULL);

  testing_write();

  ticker_running = 0;
  test_array();
  printf("\n");
  pthread_join(ticker_thread, NULL);
  return 0;
}
