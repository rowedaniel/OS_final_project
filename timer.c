/**
 * @authors Jeff Butler and Daniel Rowe
*/
#include "timer.h"
/**
 * includes for typing all from https://github.com/seL4/seL4
*/
//from include/arch/riscv/arch/types.h
typedef uint64_t timestamp_t;

//from include/stdint.h
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

//from include/arch/x86/arch/types.h and company
typedef unsigned long word_t;
typedef word_t seL4_Word;
typedef word_t cptr_t;
typedef cptr_t seL4_CPtr;
typedef seL4_CPtr seL4_IRQHandler;


/**
 * Types for timer driver
*/
typedef void (*timer_callback_t)(int);

typedef struct registers_n {
    uint32_t mux;
    uint32_t timer_a;
    uint32_t timer_b;
    uint32_t timer_c;
    uint32_t timer_d;
    uint32_t unused[13];
    uint32_t timer_e;
    uint32_t timer_e_hi;
    uint32_t mux1;
    uint32_t timer_f;
    uint32_t timer_g;
    uint32_t timer_h;
    uint32_t timer_i;
} registers_t;

typedef struct timer_n { 
    registers_t * registers;
    int enabled;
} timer_t;

timer_t timer; 

//Defines -- From Ivan's driver
#define TIMER_BASE      0xc1100000
#define TIMER_MAP_BASE  0xc1109000
#define TIMER_REG_START   0x2650    // TIMER_MUX
#define NS_IN_US 1000


//driver functions 

int start_timer(unsigned char *timer_vaddr){
    //from Ivan's driver, dont quite understand what is going on here other than that it gives us 
    //an address for the timer registers somehow.
    timer.registers = (void *)(timer_vaddr + (TIMER_BASE + TIMER_REG_START * 4 - TIMER_MAP_BASE));
    // timer.registers->mux = (TIMER_A_EN | (TIMESTAMP_TIMEBASE_1_US << TIMER_E_INPUT_CLK) |
    //                                   (TIMEOUT_TIMEBASE_1_MS << TIMER_A_INPUT_CLK));

    timer.registers->timer_e = 0;

}

timestamp_t get_time(void);

uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data);

int remove_timer(uint32_t id);

int stop_timer(void);

int timer_irq(void *data, seL4_Word irq, seL4_IRQHandler irq_handler);
