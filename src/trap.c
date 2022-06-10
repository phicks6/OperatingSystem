#include <printf.h>
#include <csr.h>
#include <plic.h>

#define SBI_PUT_CHAR 2
#define SBI_GET_CHAR 1

void os_trap_handler(void){
    long scause;
    long hartid;
    long *sscratch;
    unsigned long sepc;

    
    CSR_READ(scause, "scause");
    hartid = sbi_whoami();
    CSR_READ(sscratch,"sscratch");
    CSR_READ(sepc,"sepc");
    
    //Determine if we are here because of an asynchronous or synchronous interrupt
    int is_async = MCAUSE_IS_ASYNC(scause);
    scause &= 0xff;
    if(is_async){
        switch(scause){
            case 5:{ //timer interrupt 
                sbi_clear_timer_interrupt();
                schedule_next(hartid);
                WFI();
            }
            case 9:{ //supervisor level external interrupt
                plic_handle_irq(0); //in plic.c
            }
            break;
            default:
                printf("OS Hart %d unhandled async interrupt %ld\n\n",hartid,scause);
                WFI();
                break;
        }
    }else{
        switch(scause){
            case 8: //Ecall from user mode
                syscall_handle(hartid,sepc,sscratch); //in syscall.c
                break;
            case 12: //supervisor level external interrupt
                printf("\n\n\ninstruction Page fault on hart %d :(\n\n\n\n",hartid);
                WFI();
                break;
            case 13: //supervisor level external interrupt
                printf("\n\n\nLoad page fault on hart %d :(\n\n\n\n",hartid);
                WFI();
                break;
            case 15: //supervisor level external interrupt
                printf("\n\n\nStore/AMO page fault on hart %d :(\n\n\n\n",hartid);
                WFI();
                break;
            default:
                printf("OS Hart %d Unhandled sync interrupt %ld\n\n",hartid,scause);
                WFI();
                break;
        }
    }
    return;
}

