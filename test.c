#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
// #include "./gpio_driver.c"
//  --- ENUMS & TYPES DEFINITIONS ---
typedef enum { READING_MODE, READING_ADDRESS, READING_BYTE } ReadState;

typedef enum {
  WAITING,
  DATA_TRANSIT,
  TARGET_EXISTS,
  SEARCHING,
  DATA_RECIVED
} ModeState;

typedef enum { UNKOWN, RISING, FAILING } EdgeState;

typedef struct {
  uint8_t pin;
  uint8_t recieved_bits;
  int32_t last_falling_edge;
  int32_t last_rising_edge;
  EdgeState last_edge;
  ModeState mode;
  ReadState r;
  uint8_t scratch_buffer;
  uint8_t mode_buffer;
  uint8_t address_buffer;
  uint8_t data_buffer;
} communication_line_rx_param_t;

// --- HARDWARE SIMULATION GLOBALS ---
volatile uint32_t timer_ticks = 0;
uint8_t simulated_gpio_stream[100];
uint32_t max_stream_ticks = 0;

uint8_t read_gpiob_level(uint8_t pin) {
  if (timer_ticks < max_stream_ticks) {
    return simulated_gpio_stream[timer_ticks];
  }
  return 1; // Idle HIGH
}

// --- YOUR EXACT CHECK_THE_BITE FUNCTION ---
int check_the_bite(communication_line_rx_param_t *params_list) {
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
      printf("scratch : 0x%02X | address : 0x%02X\n",
             params_list->scratch_buffer, params_list->address_buffer);
      params_list->address_buffer = params_list->scratch_buffer;
      params_list->last_falling_edge = -1;
      params_list->recieved_bits = 0;
      printf("scratch : 0x%02X | address : 0x%02X\n",
             params_list->scratch_buffer, params_list->address_buffer);

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
        printf("check RISING \n");
        params_list->scratch_buffer =
            (params_list->scratch_buffer << 1) | (1);

        if (params_list->recieved_bits != 0) {
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

        // printf("check FAILIN \n");
        if (params_list->recieved_bits != 0) {
          if (check_the_bite(params_list) == 0) {
            params_list->last_edge = UNKOWN;
            params_list->scratch_buffer = 0;
            params_list->last_falling_edge = -1;
            break;
          } else {
            params_list->recieved_bits++;
            params_list->last_edge = RISING;
          }
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
          if (params_list->recieved_bits == 0) {
            if (check_the_bite(params_list) != 0) {
              params_list->last_edge = UNKOWN;
              params_list->last_falling_edge = -1;
              break;
            } else {
              params_list->recieved_bits++;
              params_list->last_edge = RISING;
              params_list->last_rising_edge = timer_ticks;
            }
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

// --- MAIN SIMULATION DRIVER ---
int main(void) {
  communication_line_rx_param_t rx = {0};
  rx.pin = 1;
  rx.r = READING_MODE;
  rx.last_edge = UNKOWN;
  rx.last_falling_edge = -1;

  // Load your exact signal stream levels
  uint8_t stream[] = {
      1, 1, 1, 0, 0, 1, 0,
      1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 
      1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 
      1, 1, 1,
  };
  max_stream_ticks = sizeof(stream);
  for (uint32_t i = 0; i < max_stream_ticks; i++) {
    simulated_gpio_stream[i + 2] = stream[i]; // Start at tick 2
  }
  max_stream_ticks += 2;

  printf("--- STARTING REAL CODE SIMULATION ---\n");

  uint8_t previous_pin_level = read_gpiob_level(rx.pin);

  for (timer_ticks = 2; timer_ticks < max_stream_ticks; timer_ticks++) {
    uint8_t current_pin_level = read_gpiob_level(rx.pin);

    // Only fire the handler if an edge transition (0->1 or 1->0) occurred
    if (current_pin_level != previous_pin_level) {
      Communication_Line_Default_Read(&rx);
    }

    printf("Tick %2d | Pin: %d | RecvBits: %d | State(r): %d | Mode: %d | "
           "Scratch: 0x%02X | last_edge = %d | last_falling = %d | last_rising "
           "= %d | ModeBuf: 0x%02X | AddrBuf: 0x%02X | DATABuf: 0x%02X\n",
           timer_ticks, current_pin_level, rx.recieved_bits, rx.r, rx.mode,
           rx.scratch_buffer, rx.last_edge, rx.last_falling_edge,
           rx.last_rising_edge, rx.mode_buffer, rx.address_buffer,
           rx.data_buffer);
    previous_pin_level = current_pin_level;
  }
  printf("\n--- FINAL MEMORY RESULTS ---\n");
  printf("mode_buffer    : 0x%02X\n", rx.mode_buffer);
  printf("address_buffer : 0x%02X\n", rx.address_buffer);
  printf("data_buffer    : 0x%02X\n", rx.data_buffer);

  return 0;
}
