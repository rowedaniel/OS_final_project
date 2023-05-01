/**
 * @authors Jeff Butler and Daniel Rowe
 */

#include "timer.h"
#include <stddef.h>

// ODROID-C2 timer addresses
// see https://dn.odroid.com/S905/DataSheet/S905_Public_Datasheet_V1.1.4.pdf,
// pages 100-103
#define TIMER_BASE 0xc1100000 // reg final address = TIMER_BASE + OFFSET * 4

// Taken from Ivan's code, no clue where this comes from (not mentioned anywhere
// in ODROID-C2 docs that I can find
#define TIMER_MAP_BASE 0xc1109000

#define ISA_TIMER_MUX 0x2650 // used for timers A-E
// mux addrs of import
#define TIMERD_EN (1 << 19)
#define TIMERC_EN (1 << 18)
#define TIMERB_EN (1 << 17)
#define TIMERA_EN (1 << 16)
#define TIMERD_MODE (1 << 15)
#define TIMERC_MODE (1 << 14)
#define TIMERB_MODE (1 << 13)
#define TIMERA_MODE (1 << 12)
#define TIMERE_CLK_BIT 8
#define TIMERD_CLK_BIT 6
#define TIMERC_CLK_BIT 4
#define TIMERB_CLK_BIT 2
#define TIMERA_CLK_BIT 0
// mux values of import
#define TIMEBASE_RES_1uS 0b00
#define TIMEBASE_RES_10uS 0b01
#define TIMEBASE_RES_100uS 0b10
#define TIMEBASE_RES_1mS 0b11
#define TIMERE_TIMEBASE_SYSTEM_CLOCK 0b000
#define TIMERE_TIMEBASE_RES_1uS 0b001
#define TIMERE_TIMEBASE_RES_10uS 0b010
#define TIMERE_TIMEBASE_RES_100uS 0b011
#define TIMERE_TIMEBASE_RES_1mS 0b100

// TODO: these are unused (since the register assumes they are
// sequential, in this order)
#define ISA_TIMERA 0x2651
#define ISA_TIMERB 0x2652
#define ISA_TIMERC 0x2653
#define ISA_TIMERD 0x2654
#define ISA_TIMERE 0x2662
#define ISA_TIMERE_HI 0x2663
// TODO: consider using the second MUX as overflow (it works identically to
// first, pretty sure)
#define ISA_TIMER_MUX1 0x2664 // used for timers F-I

#define TIMERA_IRQ 38
#define TIMERB_IRQ 39
#define TIMERC_IRQ 34
#define TIMERD_IRQ 57

/**
 * includes for typing all from https://github.com/seL4/seL4
 */
// from include/arch/riscv/arch/types.h

// from include/stdint.h

// from include/arch/x86/arch/types.h and company
typedef unsigned long word_t;
typedef word_t seL4_Word;
typedef word_t cptr_t;
typedef cptr_t seL4_CPtr;
typedef seL4_CPtr seL4_IRQHandler;

typedef struct {
  timer_callback_t callback;
  uint32_t running;
  uint32_t *timer_count;
} timer_unit_t;

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
  registers_t *registers;
  uint32_t unit_count;
  timer_unit_t units[4];
} timer_t;

timer_t timer;

// driver functions

int start_timer(uintptr_t timer_vaddr) {
  // timer_vaddr comes from timer.system (value is 0x2000000)
  // From ODROID-C2 docs,
  // "Each register final address = 0xC1100000 + offset * 4"
  // No clue where the TIMER_MAP_BASE comes from
  timer.registers =
      (void *)(timer_vaddr + (TIMER_BASE + ISA_TIMER_MUX * 4 - TIMER_MAP_BASE));

  // initialize all timers to miliseconds
  uint32_t settings = 0;
  // note: timers are set to ms for larger range of times
  settings |= TIMERA_EN | TIMEBASE_RES_1mS << TIMERA_CLK_BIT;
  settings |= TIMERB_EN | TIMEBASE_RES_1mS << TIMERB_CLK_BIT;
  settings |= TIMERC_EN | TIMEBASE_RES_1mS << TIMERC_CLK_BIT;
  settings |= TIMERD_EN | TIMEBASE_RES_1mS << TIMERD_CLK_BIT;
  settings |= TIMERE_TIMEBASE_RES_1uS << TIMERE_CLK_BIT;
  timer.registers->mux = settings;


  timer.unit_count = 4;
  for (uint32_t i = 0; i < timer.unit_count; ++i) {
    timer.units[i].running = 0;
  }
  timer.units[0].timer_count = &(timer.registers->timer_a);
  timer.units[1].timer_count = &(timer.registers->timer_b);
  timer.units[2].timer_count = &(timer.registers->timer_c);
  timer.units[3].timer_count = &(timer.registers->timer_d);

  // reset clock counting up
  timer.registers->timer_e = 0;

  seL4_CPtr init_thread_cnode = seL4_CapInitThreadCNode;
  seL4_CPtr irq_control = seL4_CapIRQControl;
  seL4_IRQControl_Get(irq_control, TIMERB_IRQ, init_thread_cnode, 0, seL4_WordBits);
  seL4_IRQHandler_SetNotification(0, 1);

  return 0;
}

timestamp_t get_time(void) {
  return (uint64_t)timer.registers->timer_e_hi << 32 | timer.registers->timer_e;
}

uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data) {
  for (uint32_t i = 0; i < timer.unit_count; ++i) {
    if (timer.units[i].running)
      continue;

    timer.units[i].running = 1;

    // no clue why they giving us a 64 bit int; we can only use 16 bits for the
    // timer Also, they want the delay in microseconds, but that gives us like
    // no range
    // TODO: squash the 64 bit delay into 16 bits, then put it into bits 0-15 of
    // the count
    // TODO (low priority): figure out how to shift the resolution depending on
    // how long we waiting?

    // for now, just hardcode timer to 1s;
    *(timer.units[i].timer_count) = (uint16_t)1000;

    return i;
  }

  return 0; // no available timers
}

int remove_timer(uint32_t id) {
  if (id >= timer.unit_count)
    return -1; // bad value; we only get 4 timers

  timer.units[id].running = 0;
  return 0;
}

int stop_timer(void);

/* int timer_irq(void *data, seL4_Word irq, seL4_IRQHandler irq_handler); */

// DEBUG STUFF
void test_callback(int arg) { sel4cp_dbg_puts("Callback called!\n"); }
// END DEBUG STUFF

uintptr_t timer_vaddr;
void init(void) {
  start_timer(timer_vaddr);
  register_timer(1000000, test_callback, NULL);
  register_timer(2000000, test_callback, NULL);
}

void notified(sel4cp_channel ch) {
  char buffer[2];
  sel4cp_dbg_puts("Notified! Channel is:\n");
  buffer[0] = '0' + ch;
  buffer[1] = '\n';
  sel4cp_dbg_puts(buffer);
}
