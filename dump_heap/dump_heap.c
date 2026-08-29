#include "dump_heap.h"

uint16_t destroy_dumb(my_heap_t *heap) {
  if (heap == NULL) {
    return 0;
  }
  __asm__ volatile("cpsid i" : : : "memory");
  heap->pos = heap->start;
  __asm__ volatile("cpsie i" : : : "memory");
  return 1;
}

void *allocate_dumb(my_heap_t *heap, size_t size) {
  if (heap == NULL) {
    return NULL;
  }

  __asm__ volatile("cpsid i" : : : "memory");

  size = (size + ALIGN - 1) & ~(ALIGN - 1);

  uint8_t *entry = heap->pos;

  if ((entry + size) > heap->end || (entry + size) < heap->start) {
    __asm__ volatile("cpsie i" : : : "memory");

    return NULL;
  }

  heap->pos += size;
  // __asm__ volatile("cpsie i" : : : "memory");
  return (void *)entry;
}