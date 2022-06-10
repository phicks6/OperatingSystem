#pragma once

#define MUTEX_UNLOCKED 0
#define MUTEX_LOCKED 1

typedef struct Mutex {
    int state;
} Mutex;


int mutex_trylock(struct Mutex *mutex);
void mutex_spinlock(struct Mutex *mutex);
void mutex_unlock(struct Mutex *mutex);



struct Semaphore{
    int value;
};

int semaphore_trydown(struct Semaphore *sem);
void semaphore_up(struct Semaphore *sem);



struct BarrierLinkedList{
    struct Process *proc;
    struct BarrierLinkedList *next;
};
struct Barrier{
    struct BarrierLinkedList *head;
    int value;
};



void barrier_init(struct Barrier *barrier);
void barrier_add_process(struct Barrier *barrier, struct Process *proc);
void barrier_at(struct Barrier *barrier);