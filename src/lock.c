#include <lock.h>
//#include <page.h>
#include <stddef.h>

/*
MUTEX
*/
//Returns the value
int mutex_trylock(struct Mutex *mutex){
    int old;
    //aq disables dynamic scedualing stuff
    asm volatile("amoswap.w.aq %0, %1, (%2)" 
                  : "=r"(old) 
                  : "r"(MUTEX_LOCKED), "r"(&mutex->state)
                );
    
    //we return true if the previous value was unlocked, meaning that we just locked it ourselfs
    return old != MUTEX_LOCKED;
}

//Keep trying until we lock it
void mutex_spinlock(struct Mutex *mutex){
    while (!mutex_trylock(mutex));
}

//Release the lock and we don't care about the return
void mutex_unlock(struct Mutex *mutex){
    //rl re-enables dynamic scedualing stuff
    asm volatile("amoswap.w.rl zero, zero, (%0)" 
                  : : "r"(&mutex->state)
                 );
}


//SEMAPHORE

int semaphore_trydown(struct Semaphore *sem){ //Subtracts 1 from the semaphore
    int old;
    asm volatile("amoadd.w %0, %1, (%2)" 
                    : "=r"(old) 
                    : "r"(-1), "r"(&sem->value)
                 );
    if(old <= 0){ //We always decrement so make sure we don't go below zero so our semaphore up works
        
        semaphore_up(sem);
    }
    return old > 0;
}
void semaphore_up(struct Semaphore *sem){ //Adds 1 to the semaphore unlocking it
    asm volatile("amoadd.w zero, %0, (%1)" 
                    : 
                    : "r"(1), "r"(&sem->value)
                 );
}


//BARRIER

/*void barrier_init(struct Barrier *barrier){
    barrier->head = NULL;
    barrier->value = 0;
}

void barrier_add_process(struct Barrier *barrier, struct Process *proc){
    struct BarrierLinkedList *bl;
    //TODO replace with malloc
    bl = page_phalloc(); //bl = malloc(sizeof(*bl));    
    bl->proc = proc;
    bl->next = barrier->head;
    barrier->head = bl;
    barrier->value += 1;
}

void barrier_at(struct Barrier *barrier){
    int old;
    asm volatile("amoadd.w %0, %1, (%2)" 
                   : "=r"(old) 
                   : "r"(-1), "r"(&barrier->value)
                );
    if(old <= 1){
        struct BarrierLinkedList *bl, *next;
        for (bl = barrier->head;NULL != bl;bl = next) {
            next = bl->next;
            bl->proc->state = 1; //bl->proc->state = STATE_RUNNING;
            //TODO free memory
            //free(bl);
        }
        barrier->head = NULL;
    }
}*/

