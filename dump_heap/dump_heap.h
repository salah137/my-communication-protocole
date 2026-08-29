#ifndef MY_HEAP_H
#define MY_HEAP_H

#include <stdint.h>
#include <stddef.h>

#define ALIGN 4

typedef struct {
    uint8_t *const start; 
    uint8_t *const end;
    uint8_t *pos;
    uint16_t managed;
} my_heap_t;

uint16_t destroy_dumb(my_heap_t *heap);
void *allocate_dumb(my_heap_t *heap, size_t size); 

#endif 