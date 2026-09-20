#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "test_communication_handler.h"
// Include your header definitions or mock the types here if needed
// --- Mocks & Types ---
#define my_address 0x43
#define target_address 0x23


// Globals required by your code
communication_line_t **lines;
accessible_address_t **accessible_address;
uint8_t accessible_address_count = 0;

// Mock GPIO and writing functions to intercept outputs
static uint8_t mock_gpiob_levels[10] = {1, 1, 1, 1, 1};
uint8_t read_gpiob_level(uint8_t pin) {
    if (pin < 10) return mock_gpiob_levels[pin];
    return 1;
}

static int last_written_target = -1;
static modes_t last_written_mode = (modes_t)-1;
void mock_writing_func(void *params) {
    communication_line_tx_param_t *tx = (communication_line_tx_param_t *)params;
    last_written_target = tx->address2_buffer;
    last_written_mode = tx->s_mode;
    printf("[MOCK WRITING FUNC] Target: 0x%X, Mode: %d, Data: %d\n", 
           tx->address2_buffer, tx->s_mode, tx->data_buffer);
}

// Include your functions directly or link them:
// insert_accessible_address, search_for_node, Communication_handler go here...

void setup_test_environment(void) {
    accessible_address_count = 0;
    
    // Allocate lines array (4 lines)
    lines = (communication_line_t **)malloc(sizeof(communication_line_t *) * 4);
    for (int i = 0; i < 4; i++) {
        lines[i] = (communication_line_t *)malloc(sizeof(communication_line_t));
        lines[i]->params_rx = (communication_line_rx_param_t *)calloc(1, sizeof(communication_line_rx_param_t));
        lines[i]->params_tx = (communication_line_tx_param_t *)calloc(1, sizeof(communication_line_tx_param_t));
        lines[i]->writing_func = mock_writing_func;
        lines[i]->params_tx->pin = i + 1;
    }

    // Allocate accessible address slots (16 slots)
    accessible_address = (accessible_address_t **)malloc(sizeof(accessible_address_t *) * 16);
    for (int i = 0; i < 16; i++) {
        accessible_address[i] = (accessible_address_t *)calloc(1, sizeof(accessible_address_t));
    }
}

// --- Custom Test Cases ---

void test_insert_and_search_logic(void) {
    printf("--- Running test_insert_and_search_logic ---\n");
    setup_test_environment();

    // 1. Test inserting a new address via SEARCH/TARGET discovery
    insert_accessible_address(1, 3, 0x10); // pin 1, 3 hops, address 0x10
    
    int8_t idx = search_for_node(0x10);
    assert(idx != -1);
    assert(accessible_address[idx]->address == 0x10);
    assert(accessible_address[idx]->nodes_number == 3);
    insert_accessible_address(1, 2, 0x10);

    idx = search_for_node(0x10);
    assert(accessible_address[idx]->address == 0x10);
    assert(accessible_address[idx]->nodes_number == 2);
    
    printf("count=%d idx=%d nodes_number=%d\n", accessible_address_count, search_for_node(0x10), accessible_address[search_for_node(0x10)]->nodes_number);
    
    assert(accessible_address[idx]->pin == 1);

    // 2. Test updating with a shorter path (smaller hop count)
    insert_accessible_address(2, 1, 0x10); // pin 2, 1 hop, same address 0x10
    idx = search_for_node(0x10);
    assert(accessible_address[idx]->pin == 2);
    assert(accessible_address[idx]->nodes_number == 1);

    printf("SUCCESS: Insert and Search logic passed!\n\n");
}

void test_communication_handler_transit(void) {
    printf("--- Running test_communication_handler_transit ---\n");
    setup_test_environment();

    // Pre-populate route for target 0x23 on pin 1
    insert_accessible_address(1, 2, 0x23);

    // Simulate receiving a packet meant to transit through this node to 0x23.
    // NOTE: the DATA_TRANSIT/SEARCHING/TARGET_EXISTS branches live inside the
    // `finished_reading_data == 1` switch, not the `finished_reading_address2`
    // block (that one only fires for mode == DATA_RECIVED). So the flag that
    // has to be set here is finished_reading_data, not finished_reading_address2.
    communication_line_t *test_line = lines[0];
    test_line->params_rx->finished_reading_data = 1;
    test_line->params_rx->mode = DATA_TRANSIT;
    test_line->params_rx->address2_buffer = 0x23; // Destination is 0x23 (not my_address 0x43)
    test_line->params_rx->address_buffer = 0x55;  // Original sender
    test_line->params_rx->data_buffer = 0x99;

    // Trigger handler
    Communication_handler((void *)test_line);

    // Verify that it forwarded the packet down pin 1's TX params using mock writing func
    assert(last_written_target == 0x23);
    printf("SUCCESS: Communication handler transit logic passed!\n\n");
}

void test_searching_replies_when_target_is_me(void) {
    printf("--- Running test_searching_replies_when_target_is_me ---\n");
    setup_test_environment();

    // A SEARCHING frame arrives on line 0, looking for my_address (0x43).
    communication_line_t *test_line = lines[0];
    test_line->params_rx->finished_reading_data = 1;
    test_line->params_rx->mode = SEARCHING;
    test_line->params_rx->address_buffer = 0x55;   // originator
    test_line->params_rx->address2_buffer = my_address; // target = me
    test_line->params_rx->data_buffer = 2;         // hop count so far
    test_line->params_rx->pin = 1;

    Communication_handler((void *)test_line);

    // Should reply directly with TARGET_EXISTS back on this same line.
    assert(last_written_target == my_address);
    assert(last_written_mode == TARGET_EXISTS);
    printf("SUCCESS: Searching-for-me reply logic passed!\n\n");
}

void test_searching_broadcasts_when_route_dead(void) {
    printf("--- Running test_searching_broadcasts_when_route_dead ---\n");
    setup_test_environment();

    // NOTE: Communication_handler's SEARCHING case immediately calls
    // insert_accessible_address(pin, data_buffer, address2_buffer) before
    // checking whether the target is known, so it always "learns" a route
    // to the target via the pin the frame arrived on. The flood/broadcast
    // branch therefore only triggers when read_gpiob_level() reports that
    // pin as down (0) right after that self-insert -- so that's what we
    // simulate here to reach the broadcast path.
    mock_gpiob_levels[1] = 0;

    communication_line_t *test_line = lines[0];
    test_line->params_rx->finished_reading_data = 1;
    test_line->params_rx->mode = SEARCHING;
    test_line->params_rx->address_buffer = 0x55;
    test_line->params_rx->address2_buffer = 0x99; // target
    test_line->params_rx->data_buffer = 0;
    test_line->params_rx->pin = 1;

    Communication_handler((void *)test_line);

    for (int i = 0; i < 4; i++) {
        assert(lines[i]->params_tx->s_mode == SEARCHING);
        assert(lines[i]->params_tx->address2_buffer == 0x99);
        assert(lines[i]->params_tx->data_buffer == 1); // hop count incremented
    }

    mock_gpiob_levels[1] = 1; // restore for subsequent tests
    printf("SUCCESS: Searching broadcast/flood logic passed!\n\n");
}

int main(void) {
    test_insert_and_search_logic();
    test_communication_handler_transit();
    test_searching_replies_when_target_is_me();
    test_searching_broadcasts_when_route_dead();
    printf("All custom logic tests completed successfully!\n");
    return 0;
}