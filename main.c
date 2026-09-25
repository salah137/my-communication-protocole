#include "stdint.h"
#include <stdlib.h>

extern void Init_Communication_Lines(uint8_t address);
extern int create_thread_linked(char *name, void (*thread_function)(void *),
                  uint32_t stack_size, void *params);
extern void start_routines();
extern void Fire_PendSv(uint8_t from_isr);

void task1(void* param){
    while (1) {
    
    }
}

void task2(void* param){
    while (1) {
    
    }
}

void task3(void* param){
    while (1) {
    
    }
}



int main(){
    Init_Communication_Lines(0x03);
    create_thread_linked("task1", task1, 128, NULL);
    create_thread_linked("task2", task2, 128, NULL);
    create_thread_linked("task3", task3, 128, NULL);

    start_routines();

    Fire_PendSv(0);
    
    while (1) {
    
    }

    return 0;
}