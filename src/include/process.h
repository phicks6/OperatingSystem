#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <page.h>


#define PROC_DEFAULT_CTXTMR  10000
#define PROC_DEFAULT_QUANTUM 100

#define DEFAULT_PROCESS_VADDR         0x800000
#define DEFAULT_PROCESS_STACK_POINTER 0x1cafe0000UL

typedef enum{
    PS_DEAD,
    PS_UNINIT,
    PS_RUNNING,
    PS_SLEEPING,
    PS_WAITING,
    PS_ASYNC_FREE
}ProcessState;

typedef struct ProcessFrame{
    uint64_t gpregs[32];
    double fpregs[32];
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t sie;
    uint64_t satp;
    uint64_t sscratch;
    uint64_t stvec;
    uint64_t trap_satp;
    uint64_t trap_stack;
}ProcessFrame;

typedef struct Process{
    // Keep the trap frame at the very top
    // so the offsets are easy in assembly.
    ProcessFrame frame;
    ProcessState state;
    uint64_t sleep_until;
    uint16_t quantum;
    uint16_t pid;
    void *image;
    uint64_t num_image_pages;
    void *stack;
    uint64_t num_stack_pages;
    void *heap;
    uint64_t num_heap_pages;
    struct PageTable *ptable;
    bool os_mode;
    int on_hart;
}Process;

extern Process *process_current[];
extern uint64_t process_trap_stacks[];


Process * new_process(void);
Process * new_user_process(void);
void free_process(Process *p);
bool process_spawn_on_hart(Process *p, int hart);