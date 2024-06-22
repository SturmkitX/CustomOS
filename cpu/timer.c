#include "timer.h"
#include "isr.h"
#include "ports.h"
#include "../libc/function.h"
#include "../cpu/thread.h"
#include "../libc/mem.h"

#include <stddef.h>

uint32_t tick = 0;

static uint32_t millis_per_tick = 1;    // UNINITIALIZED VALUE

static void timer_callback(registers_t *regs) {
    tick++;

    // we will schedule threads based on our ticks
    if (tick % 100 == 0) {
        kprintf("Scheduling next thread (NULL = %u)\n", getCurrentThread() == NULL);
        scheduleNext(regs);
    }
}

void init_timer(uint32_t freq) {
    /* Install the function we just wrote */
    register_interrupt_handler(IRQ0, timer_callback);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    uint32_t divisor = 1193180 / freq;
    millis_per_tick = 1000 / freq;
    uint8_t low  = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)( (divisor >> 8) & 0xFF);
    /* Send the command */
    port_byte_out(0x43, 0x36); /* Command port */
    port_byte_out(0x40, low);
    port_byte_out(0x40, high);
}

void sleep(uint32_t millis) {
    uint32_t elapsedTicks = tick;
    uint32_t neededTicks = millis / millis_per_tick;

    // busy waiting for now, since we don't have other threads
    while (tick - elapsedTicks < neededTicks) {}
}

uint32_t get_ticks() {
    return tick;
}

