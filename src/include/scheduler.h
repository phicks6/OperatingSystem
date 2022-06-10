#pragma once

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct ProcessList{
    Process * p;
    uint64_t * vruntime;
    struct ProcessList * next;
    struct ProcessList * prev;
}ProcessList;

void scheduler_init(void);
void scheduler_add_new(Process *p);
Process *scheduler_find_next(int hart);
void scheduler_park(int hart);
void schedule_next(int hart);
void scheduler_exit(int hart);