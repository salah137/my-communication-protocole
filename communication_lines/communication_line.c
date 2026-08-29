#include "../dump_heap/dump_heap.h"
#include "../tasks/tasks.h"
#include <stdint.h>

extern void *allocate_dumb(my_heap_t *heap, size_t size);
extern thread_t *create_thread(char *name, void (*thread_function)(void *),
                               uint32_t stack_size, void *params, uint16_t id);
extern my_heap_t my_heap;
extern uint8_t read_gpiob_level(uint8_t pin);
extern volatile uint32_t timer_ticks ;


uint8_t mode;

typedef struct {
  uint8_t flag;
  void *body;
  uint8_t pin;
  uint16_t buffer;
} communication_line_param_t;

typedef struct {
  communication_line_param_t *param;
  thread_t *communication_thread;
} communication_line_t;

typedef enum {
    RISING,
    FAILING,
    UNKOWN
} last_edge_t ;

typedef enum{
    READING_MODE,
    READING_BYTE
} reading_t;

reading_t r = READING_MODE;

communication_line_t **lines;
uint8_t i = 0;
int8_t last_falling_edge = -1;
int8_t last_rising_edge = -1;

last_edge_t last_edge = UNKOWN;

void Communication_Line1(void *params)
    __attribute((weak, alias("Communication_Line_Default")));
void Communication_Line2(void *params)
    __attribute((weak, alias("Communication_Line_Default")));
void Communication_Line3(void *params)
    __attribute((weak, alias("Communication_Line_Default")));
void Communication_Line4(void *params)
    __attribute((weak, alias("Communication_Line_Default")));

communication_line_t* create_line(uint8_t i){
    communication_line_param_t *params =
        allocate_dumb(&my_heap, sizeof(communication_line_param_t));
  
    params->flag = 2;
    params->body = NULL;
    params->pin = i;
    
    thread_t *thread ;

    switch (i) {
        case 1:
           thread = create_thread("line_1", Communication_Line1, 256, (void *)params, 0);
           break;

        case 2:
           thread = create_thread("line_2", Communication_Line2, 256, (void *)params, 0);
           break;

        case 3:
           thread = create_thread("line_3", Communication_Line3, 256, (void *)params, 0);
           break;

        case 4:
           thread = create_thread("line_4", Communication_Line4, 256, (void *)params, 0);
           break;


        default:
            return  0;
    }
        
  
    communication_line_t *line =
        allocate_dumb(&my_heap, sizeof(communication_line_t));
    line->communication_thread = thread;
    line->param = params;

    return  line;
  

}
    
void Init_Communication_Lines(void) {
  lines = allocate_dumb(&my_heap, sizeof(communication_line_t *) * 15);

  
  communication_line_t * l;
  
  for(int i = 1; i<=4 ; i++){
      l = create_line(i);
      if (l == 0){
          break;;
      } else {
          lines[i-1] = l;
      }
      
  }
  
}


// coock this , not finished at all 
void Communication_Line_Default(void *params) {
  communication_line_param_t *params_list =
      (communication_line_param_t *)params;

    if (params_list->flag == 0) {
      // read the data
    } else if (params_list->flag == 1) {
        if(read_gpiob_level(params_list->pin) == 0){
            // falling edge
            if(last_rising_edge != -1){
                last_falling_edge = timer_ticks;
                switch (last_edge) {
                    case RISING:
                        uint8_t bits_count = timer_ticks - last_rising_edge ;
                        for(uint8_t i = 0; i<bits_count;i++){  
                            params_list->buffer = (params_list->buffer<<1) | (1);
                            i++;

                            switch () {
                            
                            }
                        }
                        break;

                    case FAILING : 
                        // impossible
                        break;

                    case UNKOWN:
                        
                        break;
                }

                last_edge = FAILING;
                
            } else if(last_rising_edge == -1){
            
            }
        } else if(read_gpiob_level(params_list->pin == 1)){
            // rising_edge
            if(last_falling_edge != -1){

                switch (last_edge) {
                    case RISING:
                        // impossible
                        break;

                    case FAILING : 
                        uint8_t bits_count = timer_ticks - last_falling_edge ;
                        for(uint8_t i = 0; i<bits_count;i++){  
                            params_list->buffer = (params_list->buffer<<1) & ~(1);
                            i++;
                            switch (r) {
                                case READING_MODE : 
                                    if(i == 2){
                                        mode = params_list->buffer;
                                        last_falling_edge = -1;
                                        i=0;
                                    }
                                    break;
                                case READING_BYTE :
                                    if(i == 8){
                                        // 11 : packet recived raise exeption or do something
                                        if(mode & (1) && mode & (1<<2)){
                                            
                                        }
                                        // 01 : data transit
                                        else if (mode & (1) && ((mode & (1 << 2)) == 0)){

                                        }
                                        // 10 :  target exists
                                        else if (((mode & (1) )== 0) && mode & (1 << 2)){
                                            
                                        }
                                        // 00 : searching for target
                                        else if (((mode & (1) )== 0)  && (mode & (1 << 2)) == 0){
                                            
                                        }
                                    }
                                    break;
                            }
                        }
                        break;

                    case UNKOWN:
                        break;
                }

                last_edge = RISING;
                last_rising_edge = timer_ticks;
                

            } if(last_falling_edge == -1){
            
            }

        }
    
    } else {
      // return the tha  MSP
    }

    __asm__ volatile ("svc #0");

    return;
}

// 00000000; 