#include <csr.h>
#include <stdbool.h>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <lock.h>
#include <malloc.h>
#include <scheduler.h>
#include <mmu.h>

extern Process *main_idle_processes[];
extern Process *process_current[];
Mutex scheduler_mutex = {MUTEX_UNLOCKED};

ProcessList * process_list_head;
uint64_t next_pid = 1;

void scheduler_init(void){
    mutex_spinlock(&scheduler_mutex);
    process_list_head = pzalloc(sizeof(ProcessList));
    process_list_head->p = NULL;
    process_list_head->vruntime = 0;
    process_list_head->next = NULL;
    mutex_unlock(&scheduler_mutex);
}

void scheduler_add_new(Process *p){
    mutex_spinlock(&scheduler_mutex);
    p->pid = next_pid;
    next_pid++;
    p->frame.satp = (SATP_MODE_SV39 | SATP_SET_PPN(p->ptable) | SATP_SET_ASID(p->pid));
    SFENCE_ASID(p->pid);
    ProcessList* next = process_list_head->next;
    ProcessList* new = pzalloc(sizeof(ProcessList));
    new->p = p;
    new->vruntime = 0;
    new->next = next;
    new->prev = process_list_head;
    process_list_head->next = new;
    if(next!=NULL){
        next->prev = new;
    }
    p->state = PS_RUNNING;
    mutex_unlock(&scheduler_mutex);
}

//Find next process to put on a hart
Process *scheduler_find_next(int hart){
    Process *current_process = process_current[hart];
    mutex_spinlock(&scheduler_mutex);

    //Find process and update its run time
    ProcessList *pl = process_list_head;
    while(pl->next != NULL){
        pl = pl->next;
        if(pl->p == current_process){
            pl->vruntime += pl->p->quantum * PROC_DEFAULT_CTXTMR;
            break;
        }
    }
    

    //Update its place in the list based on new runtime
    ProcessList *pl_current = pl;
    if(pl->next != NULL && pl->vruntime > pl->next->vruntime){
        //printf("Need to update place\n");
        pl->prev->next = pl->next;
        pl->next->prev = pl->prev;

        pl = pl->next;
        while(pl->next != NULL && pl_current->vruntime > pl->next->vruntime){
            pl = pl->next;
        }
        if(pl->next != NULL){
            pl->next->prev = pl_current;
        }
        pl_current->next = pl->next;
        pl_current->prev = pl;
        pl->next = pl_current;
        pl_current->prev;
    }

   

    pl = process_list_head;
    while(1){
        if(pl->next == NULL){
            pl = NULL;
            break;
        }
        pl = pl->next;
        if(pl->p->state == PS_RUNNING && pl->p->on_hart == -1){
            //printf("Found a runable process\n");
            break;
        }
    }
    if(pl == NULL){
        //printf("hart %d going to idle\n",hart);
        mutex_unlock(&scheduler_mutex);
        return NULL; //Scedual idle
    }else{
        pl->p->on_hart = hart;
        mutex_unlock(&scheduler_mutex);
        return pl->p;
    }
}

//Stop a hart
void scheduler_park(int hart){
    Process *p = process_current[hart];
    if(p == NULL) {
        return;
    }
    CSR_READ(p->frame.sepc, "sepc");
    p->on_hart = -1;
}

//Put next process on hart
void schedule_next(int hart){
    scheduler_park(hart);
    Process *p = scheduler_find_next(hart);
    if (p == NULL) {
        p = main_idle_processes[hart];
    }
    process_spawn_on_hart(p,hart);
}

//Process exited so clean it up and run another one
void scheduler_exit(int hart){
    Process *cp = process_current[hart];
    cp->on_hart = -1;
    //Find process
    mutex_spinlock(&scheduler_mutex);
    ProcessList *pl = process_list_head;
    while(pl->next != NULL){
        pl = pl->next;
        if(pl->p == cp){
            if(pl->next != NULL){
                pl->next->prev = pl->prev;
            }
            pl->prev->next = pl->next;
        }
    }
    mutex_unlock(&scheduler_mutex);
    free_process(cp);
    schedule_next(hart);
}