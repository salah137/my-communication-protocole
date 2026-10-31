/*
 * Custom GPIO protocol frame simulator - ESP-IDF (tested layout: classic ESP32)
 *
 * Both lines idle HIGH through an external 5.1k pull-up to 3.3V, so TX is driven
 * open-drain: "0" = pull the line low, "1" = release it and let the resistor
 * pull it up. RX is a plain input with edge interrupts (no internal pulls).
 *
 * The drawings do not show how a 0 differs from a 1, or how the 2 mode bits sit
 * inside the header, so the on-wire encoding below is my choice. Everything that
 * defines it is in the timing block, change it there to match your receiver.
 *
 *   header : line LOW for  1500 + mode*500 us   (1500/2000/2500/3000), then HIGH gap
 *   bit    : line LOW for  200 us = 0 or 500 us = 1, then HIGH 200 us   (MSB first)
 *   field  : 8 bits, then an extra HIGH gap before the next field
 *   frame  : header + fields; idle HIGH afterwards ends the frame
 *
 * Decoding only needs falling->rising width, so it does not depend on the slow
 * pull-up rise time or on a following edge after the last bit.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

/* ------------------------------ pins ------------------------------------ */
#define TX_PIN              GPIO_NUM_25   /* we drive this line (open-drain)   */
#define RX_PIN              GPIO_NUM_26   /* the other node drives this line   */

/* ------------------------------ timing (us) ------------------------------ */
#define T_HDR_LOW_BASE_US   1500
#define T_HDR_LOW_STEP_US   500
#define T_HDR_GAP_US        1000
#define T_BIT_LOW_0_US      200
#define T_BIT_LOW_1_US      500
#define T_BIT_HIGH_US       200
#define T_FIELD_GAP_US      1000

/* receiver tolerances */
#define T_BIT_MIN_US        100           /* shorter than this = glitch        */
#define T_BIT_THRESHOLD_US  350           /* <= 0, > 1                         */
#define T_BIT_MAX_US        800           /* longer than this = not a bit      */
#define FRAME_END_TIMEOUT_MS 20           /* idle this long = frame finished   */
#define RESPONSE_TIMEOUT_MS  500          /* wait this long for a response     */

/* ------------------------------ frame model ------------------------------ */
typedef enum {
    MODE_SEARCH        = 0,   /* "00" */
    MODE_DATA_TRANSIT  = 1,   /* "01" */
    MODE_TARGET_EXISTS = 2,   /* "10" */
    MODE_DATA_RECEIVED = 3,   /* "11" */
} proto_mode_t;

typedef struct {
    proto_mode_t mode;
    uint8_t      field[3];
    uint8_t      nfields;
} frame_t;

static const uint8_t NFIELDS[4] = { 3, 3, 3, 2 };

static const char *const MODE_NAMES[4] = {
    "SEARCH (00)", "DATA TRANSIT (01)", "TARGET EXISTS (10)", "DATA RECEIVED (11)",
};

static const char *const LABELS[4][3] = {
    [MODE_SEARCH]        = { "target",   "searcher",   "nodes passed" },
    [MODE_DATA_TRANSIT]  = { "target",   "source",     "data"         },
    [MODE_TARGET_EXISTS] = { "searcher", "found node", "nodes passed" },
    [MODE_DATA_RECEIVED] = { "receiver", "source",     ""             },
};

static void print_frame(const char *dir, const frame_t *f)
{
    printf("%s %s\n", dir, MODE_NAMES[f->mode]);
    for (int i = 0; i < f->nfields; i++) {
        printf("      %-13s = 0x%02X (%u)\n", LABELS[f->mode][i], f->field[i], f->field[i]);
    }
}

/* ------------------------------ RX side ---------------------------------- */
typedef struct {
    uint32_t t_us;
    uint8_t  level;
} edge_t;

typedef enum { RX_OK, RX_TIMEOUT, RX_BAD_FRAME } rx_result_t;

static QueueHandle_t edge_q;

static void IRAM_ATTR rx_isr(void *arg)
{
    edge_t e = {
        .t_us  = (uint32_t)esp_timer_get_time(),
        .level = (uint8_t)gpio_get_level(RX_PIN),
    };
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(edge_q, &e, &woken);
    if (woken) {
        portYIELD_FROM_ISR();
    }
}

static bool decode_header(uint32_t w, proto_mode_t *m)
{
    if (w + T_HDR_LOW_STEP_US / 2 < T_HDR_LOW_BASE_US) {
        return false;
    }
    uint32_t idx = (w + T_HDR_LOW_STEP_US / 2 - T_HDR_LOW_BASE_US) / T_HDR_LOW_STEP_US;
    if (idx > 3) {
        return false;
    }
    *m = (proto_mode_t)idx;
    return true;
}

static int decode_bit(uint32_t w)
{
    if (w < T_BIT_MIN_US)       return -1;
    if (w <= T_BIT_THRESHOLD_US) return 0;
    if (w <= T_BIT_MAX_US)      return 1;
    return -1;
}

/* Blocks until a whole frame has arrived (or nothing arrives within the timeout). */
static rx_result_t rx_frame(frame_t *out, uint32_t first_edge_timeout_ms)
{
    memset(out, 0, sizeof(*out));

    edge_t e;
    if (xQueueReceive(edge_q, &e, pdMS_TO_TICKS(first_edge_timeout_ms)) != pdTRUE) {
        return RX_TIMEOUT;
    }

    bool     in_low = false, got_header = false, bad = false;
    uint32_t t_fall = 0;
    int      bit_cnt = 0, field_cnt = 0;
    uint8_t  cur = 0;

    do {
        if (e.level == 0) {
            in_low = true;
            t_fall = e.t_us;
        } else if (in_low) {
            in_low = false;
            uint32_t w = e.t_us - t_fall;

            if (bad) {
                /* keep draining so stale edges don't leak into the next frame */
            } else if (!got_header) {
                if (decode_header(w, &out->mode)) {
                    got_header   = true;
                    out->nfields = NFIELDS[out->mode];
                } else {
                    bad = true;
                }
            } else if (field_cnt >= out->nfields) {
                bad = true;                         /* more bits than the mode allows */
            } else {
                int b = decode_bit(w);
                if (b < 0) {
                    bad = true;
                } else {
                    cur = (uint8_t)((cur << 1) | b);
                    if (++bit_cnt == 8) {
                        out->field[field_cnt++] = cur;
                        bit_cnt = 0;
                        cur = 0;
                    }
                }
            }
        }
    } while (xQueueReceive(edge_q, &e, pdMS_TO_TICKS(FRAME_END_TIMEOUT_MS)) == pdTRUE);

    if (bad || !got_header || in_low || bit_cnt != 0 || field_cnt != out->nfields) {
        return RX_BAD_FRAME;
    }
    return RX_OK;
}

/* ------------------------------ TX side ---------------------------------- */
static portMUX_TYPE tx_mux = portMUX_INITIALIZER_UNLOCKED;

static inline void line_low(void)  { gpio_set_level(TX_PIN, 0); }
static inline void line_high(void) { gpio_set_level(TX_PIN, 1); }  /* released, 5.1k pulls up */

static void tx_low_pulse(uint32_t low_us, uint32_t high_us)
{
    line_low();
    esp_rom_delay_us(low_us);
    line_high();
    esp_rom_delay_us(high_us);
}

static void tx_header(proto_mode_t m)
{
    tx_low_pulse(T_HDR_LOW_BASE_US + (uint32_t)m * T_HDR_LOW_STEP_US, T_HDR_GAP_US);
}

static void tx_byte(uint8_t v)
{
    for (int i = 7; i >= 0; i--) {
        bool one = (v >> i) & 1;
        tx_low_pulse(one ? T_BIT_LOW_1_US : T_BIT_LOW_0_US, T_BIT_HIGH_US);
    }
    esp_rom_delay_us(T_FIELD_GAP_US);               /* line stays released (high) */
}

static void tx_frame(const frame_t *f)
{
    print_frame("TX", f);

    xQueueReset(edge_q);                            /* drop anything stale before we start */

    /* Worst case ~25 ms with interrupts off on this core, fine for the watchdogs. */
    portENTER_CRITICAL(&tx_mux);
    tx_header(f->mode);
    for (int i = 0; i < f->nfields; i++) {
        tx_byte(f->field[i]);
    }
    portEXIT_CRITICAL(&tx_mux);
}

/* One function per frame type, as drawn ----------------------------------- */
void send_search(uint8_t target, uint8_t searcher, uint8_t nodes_passed)
{
    frame_t f = { MODE_SEARCH, { target, searcher, nodes_passed }, 3 };
    tx_frame(&f);
}

void send_data_transit(uint8_t target, uint8_t source, uint8_t data)
{
    frame_t f = { MODE_DATA_TRANSIT, { target, source, data }, 3 };
    tx_frame(&f);
}

void send_target_exists(uint8_t searcher, uint8_t found_node, uint8_t nodes_passed)
{
    frame_t f = { MODE_TARGET_EXISTS, { searcher, found_node, nodes_passed }, 3 };
    tx_frame(&f);
}

void send_data_received(uint8_t receiver, uint8_t source)
{
    frame_t f = { MODE_DATA_RECEIVED, { receiver, source, 0 }, 2 };
    tx_frame(&f);
}

/* ------------------------- response handling ----------------------------- */
#define EXPECT_NONE   (-1)   /* no response expected: silence is a pass */
#define DONT_CHECK    (-1)

static bool expect_response(const char *tag, int expect_mode, int exp0, int exp1)
{
    frame_t f;
    rx_result_t r = rx_frame(&f, RESPONSE_TIMEOUT_MS);

    if (expect_mode == EXPECT_NONE) {
        if (r == RX_TIMEOUT) {
            printf("[%s] RESULT: OK, no response expected and none seen\n\n", tag);
            return true;
        }
        printf("[%s] RESULT: FAIL, unexpected traffic on RX\n", tag);
        if (r == RX_OK) print_frame("RX", &f);
        printf("\n");
        return false;
    }

    if (r == RX_TIMEOUT) {
        printf("[%s] RESULT: FAIL, no response within %d ms\n\n", tag, RESPONSE_TIMEOUT_MS);
        return false;
    }
    if (r == RX_BAD_FRAME) {
        printf("[%s] RESULT: FAIL, malformed frame (bad timing or wrong bit count)\n\n", tag);
        return false;
    }

    print_frame("RX", &f);

    if ((int)f.mode != expect_mode) {
        printf("[%s] RESULT: FAIL, got %s, expected %s\n\n",
               tag, MODE_NAMES[f.mode], MODE_NAMES[expect_mode]);
        return false;
    }
    if ((exp0 != DONT_CHECK && f.field[0] != exp0) ||
        (exp1 != DONT_CHECK && f.field[1] != exp1)) {
        printf("[%s] RESULT: FAIL, right frame type but wrong addresses\n\n", tag);
        return false;
    }

    printf("[%s] RESULT: OK, %s received with matching addresses\n\n", tag, MODE_NAMES[f.mode]);
    return true;
}

/* ------------------------------ setup ------------------------------------ */
static void gpio_setup(void)
{
    edge_q = xQueueCreate(128, sizeof(edge_t));

    gpio_config_t tx_cfg = {
        .pin_bit_mask = 1ULL << TX_PIN,
        .mode         = GPIO_MODE_OUTPUT_OD,        /* external 5.1k does the pull-up */
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&tx_cfg);
    line_high();

    gpio_config_t rx_cfg = {
        .pin_bit_mask = 1ULL << RX_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&rx_cfg);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(RX_PIN, rx_isr, NULL);
}

/* ------------------------------ demo ------------------------------------- */
void app_main(void)
{
    const uint8_t MY_ADDR     = 0x01;
    const uint8_t TARGET_ADDR = 0x2A;
    const uint8_t OTHER_ADDR  = 0x10;

    gpio_setup();
    vTaskDelay(pdMS_TO_TICKS(500));

    int pass = 0, total = 0;

    printf("=== 1) search for target, expect TARGET EXISTS ===\n");
    send_search(TARGET_ADDR, MY_ADDR, 0);
    pass += expect_response("search", MODE_TARGET_EXISTS, MY_ADDR, TARGET_ADDR); total++;
    vTaskDelay(pdMS_TO_TICKS(200));

    printf("=== 2) data transit, expect DATA RECEIVED ===\n");
    send_data_transit(TARGET_ADDR, MY_ADDR, 0xA5);
    pass += expect_response("data", MODE_DATA_RECEIVED, TARGET_ADDR, MY_ADDR); total++;
    vTaskDelay(pdMS_TO_TICKS(200));

    /* The next two expectations are my guess at the protocol flow, edit freely. */
    printf("=== 3) answer a search with TARGET EXISTS, expect DATA TRANSIT ===\n");
    send_target_exists(OTHER_ADDR, MY_ADDR, 1);
    pass += expect_response("exists", MODE_DATA_TRANSIT, MY_ADDR, OTHER_ADDR); total++;
    vTaskDelay(pdMS_TO_TICKS(200));

    printf("=== 4) acknowledge with DATA RECEIVED, expect silence ===\n");
    send_data_received(MY_ADDR, OTHER_ADDR);
    pass += expect_response("ack", EXPECT_NONE, DONT_CHECK, DONT_CHECK); total++;

    printf("=== SUMMARY: %d/%d exchanges passed ===\n", pass, total);
}