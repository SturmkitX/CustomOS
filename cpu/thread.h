#ifndef _THREAD_H
#define _THREAD_H

#include <stdint.h>
#include "isr.h"

struct thread_t {
    registers_t regs;
    uint8_t priority;
    uint32_t id;
};

void addThread(struct thread_t* thread, void (*fun)());
void scheduleNext(registers_t* dest);
struct thread_t* getCurrentThread();
uint32_t fork();
void fork_callback(registers_t *regs);
uint8_t is_fork_pending();
void set_fork_complete();

#endif
