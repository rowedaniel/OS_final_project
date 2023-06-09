/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <sel4cp.h>

/*
 * This timer driver is not written by me, it comes
 * from the seL4 "util_libs" repository.
 */
#define TIMER_E_INPUT_CLK 8
#define TIMER_D_INPUT_CLK 6
#define TIMER_C_INPUT_CLK 4
#define TIMER_B_INPUT_CLK 2
#define TIMER_A_INPUT_CLK 0

#define TIMER_D_EN      (1 << 19)
#define TIMER_C_EN      (1 << 18)
#define TIMER_B_EN      (1 << 17)
#define TIMER_A_EN      (1 << 16)
#define TIMER_D_MODE    (1 << 15)
#define TIMER_C_MODE    (1 << 14)
#define TIMER_B_MODE    (1 << 13)
#define TIMER_A_MODE    (1 << 12)

#define TIMER_I_EN      (1 << 19)
#define TIMER_H_EN      (1 << 18)
#define TIMER_G_EN      (1 << 17)
#define TIMER_F_EN      (1 << 16)
#define TIMER_I_MODE    (1 << 15)
#define TIMER_H_MODE    (1 << 14)
#define TIMER_G_MODE    (1 << 13)
#define TIMER_F_MODE    (1 << 12)

#define TIMER_I_INPUT_CLK 6
#define TIMER_H_INPUT_CLK 4
#define TIMER_G_INPUT_CLK 2
#define TIMER_F_INPUT_CLK 0

#define TIMESTAMP_TIMEBASE_SYSTEM   0b000
#define TIMESTAMP_TIMEBASE_1_US     0b001
#define TIMESTAMP_TIMEBASE_10_US    0b010
#define TIMESTAMP_TIMEBASE_100_US   0b011
#define TIMESTAMP_TIMEBASE_1_MS     0b100

#define TIMEOUT_TIMEBASE_1_US   0b00
#define TIMEOUT_TIMEBASE_10_US  0b01
#define TIMEOUT_TIMEBASE_100_US 0b10
#define TIMEOUT_TIMEBASE_1_MS   0b11

#define TIMER_A_IRQ 42
#define TIMER_B_IRQ 43
#define TIMER_C_IRQ 38
#define TIMER_D_IRQ 61

#define TIMER_F_IRQ 92
#define TIMER_G_IRQ 93
#define TIMER_H_IRQ 94
#define TIMER_I_IRQ 95

typedef struct {
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
} meson_timer_reg_t;

typedef struct {
    volatile meson_timer_reg_t *regs;
    bool disable;
} meson_timer_t;

#define TIMER_BASE      0xc1100000
#define TIMER_MAP_BASE  0xc1109000

#define TIMER_REG_START   0x2650    // TIMER_MUX

#define NS_IN_US 1000

uintptr_t timer_vaddr;

meson_timer_t timer;

#define TIMER_IRQ_CH 1

uint16_t (*timer_callback)(void);

static char
hexchar(unsigned int v)
{
    return v < 10 ? '0' + v : ('a' - 10) + v;
}

static void
puthex64(uint64_t val)
{
    char buffer[16 + 3];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[16 + 3 - 1] = 0;
    for (unsigned i = 16 + 1; i > 1; i--) {
        buffer[i] = hexchar(val & 0xf);
        val >>= 4;
    }
    sel4cp_dbg_puts(buffer);
}

uint64_t meson_get_time()
{
    uint64_t initial_high = timer.regs->timer_e_hi;
    uint64_t low = timer.regs->timer_e;
    uint64_t high = timer.regs->timer_e_hi;
    if (high != initial_high) {
        low = timer.regs->timer_e;
    }

    uint64_t ticks = (high << 32) | low;
    uint64_t time = ticks * NS_IN_US;
    return time;
}

void meson_set_timeout(uint16_t timeout, bool periodic, uint16_t (*f)(void))
{
    if (periodic) {
        timer.regs->mux |= TIMER_A_MODE;
    } else {
        timer.regs->mux &= ~TIMER_A_MODE;
    }

    timer.regs->timer_a = timeout;

    if (timer.disable) {
        timer.regs->mux |= TIMER_A_EN;
        timer.disable = false;
    }
    timer_callback = f;
}

void meson_stop_timer()
{
    timer.regs->mux &= ~TIMER_A_EN;
    timer.disable = true;
}

uint16_t example_timer_callback(void)
{
	static int  numCalls = 0;
	numCalls ++;
	sel4cp_dbg_puts("Timer callback called!\n");
	sel4cp_dbg_puts("Current time is: ");
	puthex64(meson_get_time());
	sel4cp_dbg_puts("\nnumCalls: ");
	puthex64(numCalls);
	sel4cp_dbg_puts("\n");
	return 0;
}

void init()
{
    timer.regs = (void *)(timer_vaddr + (TIMER_BASE + TIMER_REG_START * 4 - TIMER_MAP_BASE));

    timer.regs->mux = TIMER_A_EN | (TIMESTAMP_TIMEBASE_1_US << TIMER_E_INPUT_CLK) |
                       (TIMEOUT_TIMEBASE_1_MS << TIMER_A_INPUT_CLK);

    timer.regs->timer_e = 0;

    // Have a timeout of 1 second, and have it be periodic so that it will keep recurring.
    sel4cp_dbg_puts("Setting a timeout of 1 second.\n");
    meson_set_timeout(1000, true, example_timer_callback);
}

void notified(sel4cp_channel ch) {
    switch (ch) {
        case TIMER_IRQ_CH:
            sel4cp_dbg_puts("Got timer interrupt!\n");
            sel4cp_irq_ack(ch);
            sel4cp_dbg_puts("Current time is: ");
            puthex64(meson_get_time());
            sel4cp_dbg_puts("\n");
	    timer_callback();
            break;
        default:
            sel4cp_dbg_puts("TIMER|ERROR: unexpected channel!\n");
    }
}
