#include <process.h>
#include <malloc.h>
#include <csr.h>
#include <sbi.h>
#include <mmu.h>

extern uint64_t spawn_thread_start;
extern uint64_t spawn_thread_end;


extern uint64_t spawn_trap_start;
extern uint64_t spawn_trap_end;

Process *process_current[MAXCPU];

Process * new_process(){
    Process * p = pzalloc(sizeof(Process));

    //Initial process state means it is new process that hasn't be run yet
    p->state = PS_UNINIT;
    //Set up page table
    p->ptable = page_phalloc();

    //Set quantum
    p->quantum = PROC_DEFAULT_QUANTUM;

    //TODO hand out valid pid
    p->pid = 68;

    //Give process 1 page for stack
    p->stack = page_phalloc();
    p->num_stack_pages = 1;

    uint64_t pAddr = mmu_translate(kernel_mmu_table, spawn_thread_start);
    mmu_map_multiple(p->ptable, align_up(spawn_thread_start), pAddr, align_down(spawn_thread_end) - align_up(spawn_thread_start), PB_EXECUTE);
    pAddr = mmu_translate(kernel_mmu_table, spawn_trap_start);
    mmu_map_multiple(p->ptable, align_up(spawn_trap_start), pAddr, align_down(spawn_trap_end) - align_up(spawn_trap_start), PB_EXECUTE);
    pAddr = mmu_translate(kernel_mmu_table, &p->frame);
    mmu_map_multiple(p->ptable , align_up((uint64_t)&p->frame), pAddr, align_down((uint64_t)&p->frame+sizeof(ProcessFrame)) - align_up((uint64_t)&p->frame), PB_READ | PB_WRITE);
    p->frame.gpregs[XREG_SP] = DEFAULT_PROCESS_STACK_POINTER + p->num_stack_pages * PAGE_SIZE;
    
    //set interupts
    p->frame.sie = SIE_SEIE | SIE_SSIE | SIE_STIE;
    //set the frame into the sscratch
    p->frame.sscratch = (uint64_t)&p->frame;
    //set location of the trap handler
    p->frame.stvec = spawn_trap_start;

    p->frame.trap_satp = (SATP_MODE_SV39 | SATP_SET_ASID(KERNEL_ASID) | SATP_SET_PPN(kernel_mmu_table));
    p->frame.satp = (SATP_MODE_SV39 | SATP_SET_ASID(1) | SATP_SET_PPN(p->ptable));
   
    //created process not on a hart yet
    p->on_hart = -1;
    
    return p;
}

//Wrappers to set fields that will differ for other types of processes
Process * new_user_process(){
    Process * p = new_process();
    
    p->os_mode = false;
    p->frame.sstatus = SSTATUS_FS_INITIAL | SSTATUS_SPIE | SSTATUS_SPP_USER;
    uint64_t pAddr = (uint64_t)p->stack;
    mmu_map_multiple(p->ptable, DEFAULT_PROCESS_STACK_POINTER, pAddr, p->num_stack_pages * PAGE_SIZE, PB_USER | PB_READ | PB_WRITE);
    return p;
}
Process *process_new_os(){
    Process *p = new_process();
    p->os_mode = true;
    p->frame.sstatus = SSTATUS_FS_INITIAL | SSTATUS_SPIE | SSTATUS_SPP_SUPERVISOR;
    uint64_t pAddr = (uint64_t)p->stack;
    mmu_map_multiple(p->ptable, DEFAULT_PROCESS_STACK_POINTER, pAddr, p->num_stack_pages * PAGE_SIZE, PB_READ | PB_WRITE);
    return p;
}


void free_process(Process *p){
    p->state = PS_DEAD;
    page_free(p->stack);
    page_free(p->image);
    mmu_free(p->ptable);
    pfree(p);

}

bool process_spawn_on_hart(Process *p, int hart){
    int whoami = -1;
    whoami = sbi_whoami();
    //printf("I am hart %d\n");

    //Get status of hart
    int s = sbi_hart_get_status(hart);
    
    //If the hart isn't us and its not ready then we can't use it
    if(whoami != hart && s != 1){
        printf("Not ready?? hart %d has status %d\n",hart,s);
        return false;
    }

    //"Put" the process on hart
    p->on_hart = hart;
    
    //Give the trap_stack a preemtively allocated stack 
    p->frame.trap_stack = process_trap_stacks[hart];
    process_current[hart] = p;

    //Do timer things
    sbi_interrupt_in(hart, p->quantum * PROC_DEFAULT_CTXTMR);


    if(whoami == hart){
        //If we are the hart we are trying to start it on we can skip the sbi
        CSR_WRITE("sscratch", p->frame.sscratch);
        ((void (*)(void))spawn_thread_start)();
        //shouldn't make it here
        return true;
    }
    //SBI needs physical addresses of the sscratch
    uint64_t pAddr = mmu_translate(p->ptable, p->frame.sscratch);
    
    // Ask the SBI to start the HART.
    return sbi_hart_start(hart, spawn_thread_start, pAddr);
}
