#include "thread.h"
#include "../libc/mem.h"
#include <stddef.h>

#define THREAD_MAX_NUM  32
#define THREAD_STACK_START  0x005A0000

// Will use a ring buffer for now. WILL switch to something like a linked list later
static struct thread_t _threads[THREAD_MAX_NUM];
static uint8_t _thread_curr = 0, _thread_size = 0;

static uint32_t lastThreadIndex = 0;

static struct thread_t* _current_thread = NULL;

void addThread(struct thread_t* thread, void (*fun)()) {
    uint8_t size = _thread_size++;
    memory_copy(&_threads[size], thread, sizeof(struct thread_t));
    if (_thread_size == THREAD_MAX_NUM) {
        kprint("Reached maximum number of threads. Bypassing...\n");
        _thread_size = 0;
    }

    if (fun != NULL) {
        _threads[size].regs.eip = (uint32_t)fun;
        _threads[size].regs.esp = THREAD_STACK_START + size * 0x1000;    // 4K stack size for now
    }
}

void scheduleNext(registers_t* dest) {
    if (_current_thread == NULL) {
        _current_thread = &_threads[0];
        _thread_size = 1;
        _current_thread->priority = 1;
        _current_thread->id = 1;
    }

    struct thread_t *entry = &_threads[_thread_curr];
    addThread(_current_thread, NULL);

    if (_thread_curr == _thread_size) {
        kprint("Thread list empty!!\n");
        asm("hlt");
    }

    _thread_curr++;
    if (_thread_curr == THREAD_MAX_NUM) {
        _thread_curr = 0;
    }

    memory_copy(dest, &entry->regs, sizeof(registers_t));
    _current_thread = entry;
}

struct thread_t* getCurrentThread() {
    return _current_thread;
}

uint32_t fork() {
    uint32_t originalId = _current_thread->id;
    asm("int $0x91");

    return (_current_thread->id - originalId);
}

static void fork_callback(registers_t *regs) {
    kprint("Got Thread Fork Interrupt (0x91)!!\n");

    uint8_t size = _thread_size++;
    _threads[size].priority = _current_thread->priority;
    _threads[size].id = ++lastThreadIndex;
    memory_copy(&_threads[size].regs, regs, sizeof(registers_t));
    if (_thread_size == THREAD_MAX_NUM) {
        kprint("Reached maximum number of threads. Bypassing...\n");
        _thread_size = 0;
    }

    uint32_t stackSize = regs->ebp - regs->esp;
    _threads[size].regs.ebp = THREAD_STACK_START + size * 0x8000;    // 32K stack size for now
    _threads[size].regs.esp = THREAD_STACK_START + size * 0x8000 - stackSize;
    memory_copy(_threads[size].regs.esp, _threads[size].regs.ebp, stackSize);
}

void init_thread() {
    /* Install the function we just wrote */
    register_soft_interrupt_handler(FORKIRQ, fork_callback);
}
