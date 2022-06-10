#include <csr.h>
#include <printf.h>
#include <plic.h>
#include <svccall.h>
#include <clint.h>

void c_trap_handler(void){
    long mcause;
    long mhartid;
    long *mscratch;
    unsigned long mepc;
    unsigned long sip;
    
    CSR_READ(mcause, "mcause");
    CSR_READ(mhartid,"mhartid");
    CSR_READ(mscratch,"mscratch");
    CSR_READ(mepc,"mepc");
 
    //Deturmine if we are here because of an asynchronous or synchronous interupt
    int is_async = MCAUSE_IS_ASYNC(mcause);
    mcause &= 0xff;
    if(is_async){
        switch(mcause){
            case 3: //Machine software interupt pending
                hart_handle_msip(mhartid);
                printf("After handle msip\n\n\n\n");
                WFI();
                break;
            case 7: //Machine Timer interupt pending
                //printf("SBI Hart %d timer\n",mhartid);
                CSR_READ(sip, "mip");
				CSR_WRITE("mip", SIP_STIP);
                clint_set_mtimecmp(mhartid,CLINT_MTIMECMP_INFINITE);
                break;
            case 11: //Machine level external interupt
                plic_handle_irq(mhartid); //in plic.c
                break;
            default:
                printf("Unhandled async interupt %ld\n\n",mcause);
                break;
        }
    }else{
        switch(mcause){
            case 9: //Ecall from supervisor mode
                svccall_handle(mhartid); //in svccall.c
                //printf("Handled svccall in sbi\n");
                CSR_WRITE("mepc", mepc+4); //Jumps to instruction after ecall so that we don't get stuck repeating the same ecall
                break;
            default:
                printf("SBI Unhandled sync interupt %ld\n\n",mcause);
                WFI();
                break;
        }
    }
}