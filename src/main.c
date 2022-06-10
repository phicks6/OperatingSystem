#include <sbi.h>
#include <printf.h>
#include <csr.h>
#include <util.h>
#include <console.h>
#include <malloc.h>
#include <symbols.h>
#include <page.h>
#include <mmu.h>
#include <plic.h>
#include <rng.h>
#include <elf.h>
#include <scheduler.h>
#include <minix3.h>
#include <fs.h>
#include <paint.h>

long int SBI_GPREGS[32]; //long int SBI_GPREGS[8][32];

uint64_t process_trap_stacks[MAXCPU];
Process *main_idle_processes[MAXCPU];

static void trap_stacks_init(void){
    // Set up the trap stacks
    for (int i = 0; i < MAXCPU; i++) {
        process_trap_stacks[i] = (uint64_t)page_phalloc();
    }
}

//creates idle processes to harts that aren't doing anything can be kept awake
static void add_idle_procs(void){
    extern uint64_t idleproc;
    extern uint64_t idleprocsize;
    Process *idle;  // = process_new_os();

    for (int i = 0; i < MAXCPU; i++) {
        main_idle_processes[i] = idle = process_new_os();

        // We don't need a stack for idle
        idle->image                   = idle->stack;
        idle->num_image_pages         = 1;
        idle->num_stack_pages         = 0;
        idle->pid                     = 0xfffe - i;
        idle->stack                   = NULL;

        memcpy(idle->image, (void *)idleproc, idleprocsize);

        mmu_map(idle->ptable, DEFAULT_PROCESS_STACK_POINTER, (uint64_t)idle->image,
                PB_EXECUTE);

        idle->frame.sepc = DEFAULT_PROCESS_STACK_POINTER;
        // The default quantum is 100. 
        idle->quantum = 2500; //Set very high to make more human readable;
        idle->frame.satp = (SATP_MODE_SV39 | SATP_SET_PPN(idle->ptable) | SATP_SET_ASID(idle->pid));
        
        SFENCE_ASID(idle->pid);
    }
}

//Designates hart 0 as the one that recieves inturupts form pcie devices
void plic_init(){
    mmu_map_multiple(kernel_mmu_table, PLIC_BASE, PLIC_BASE, 0x300000, PB_READ | PB_WRITE);//PCIe
    plic_enable(0,32); //have hart 0 enable 32
    plic_set_priority(32,6); 
    plic_set_threshold(0,0); //Have hart 0 see all above 0

    plic_enable(0,33); //have hart 0 enable 32
    plic_set_priority(33,6); 
    plic_set_threshold(0,0); //Have hart 0 see all above 0

    plic_enable(0,34); //have hart 0 enable 32
    plic_set_priority(34,6); 
    plic_set_threshold(0,0); //Have hart 0 see all above 0

    plic_enable(0,35); //have hart 0 enable 32
    plic_set_priority(35,6); 
    plic_set_threshold(0,0); //Have hart 0 see all above 0
}


//This is our OS code
int main(int hart){
    page_init();

    PageTable *pt = page_phalloc();
	kernel_mmu_table = pt;

    //Map memory for the kernel
	mmu_map_multiple(pt, sym_start(text), sym_start(text), (sym_end(text)-sym_start(text)), PB_READ | PB_WRITE | PB_EXECUTE);
	mmu_map_multiple(pt, sym_start(data), sym_start(data), (sym_end(data)-sym_start(data)), PB_READ | PB_WRITE | PB_EXECUTE);
	mmu_map_multiple(pt, sym_start(rodata), sym_start(rodata), (sym_end(rodata)-sym_start(rodata)), PB_READ | PB_WRITE | PB_EXECUTE);
	mmu_map_multiple(pt, sym_start(bss), sym_start(bss), (sym_end(bss)-sym_start(bss)), PB_READ | PB_WRITE | PB_EXECUTE);
	mmu_map_multiple(pt, sym_start(stack), sym_start(stack), (sym_end(stack)-sym_start(stack)), PB_READ | PB_WRITE | PB_EXECUTE);
	mmu_map_multiple(pt, sym_start(heap), sym_start(heap), (sym_end(heap)-sym_start(heap)), PB_READ | PB_WRITE | PB_EXECUTE);

    //Map memory for MMIO 
	mmu_map_multiple(pt, 0x0C000000, 0x0C000000, 0x0C2FFFFF - 0x0C000000, PB_READ | PB_WRITE); //PLIC
	mmu_map_multiple(pt, 0x30000000, 0x30000000, 0x10000000, PB_READ | PB_WRITE);//PCIe
	

    CSR_WRITE("satp", SATP_MODE_SV39 | SATP_SET_ASID(KERNEL_ASID) | SATP_SET_PPN(pt)); //Enable the MMU
	sfence();

    CSR_WRITE("sscratch",SBI_GPREGS);

    //Set up functionalities of the operating system
    plic_init();
    init_kernel_heap();//Allocate started memory for heap
    init_pcie(0);
    trap_stacks_init();
    scheduler_init();
    vfs_init();

    add_idle_procs();

    //Load the user application paint.c from the disk and schedual it
    int file_size = filesize("/paint.elf");
    void * paint_buff = pzalloc(file_size);
    read_from_file("/paint.elf",paint_buff,0,file_size);
    Process *p;
    p = new_user_process();
    readElf(p,paint_buff);
    scheduler_add_new(p);
    process_spawn_on_hart(main_idle_processes[2], 2);
    
    
    
    
    make_blank_canvas();
    track_mouse();
    return 0;
}


