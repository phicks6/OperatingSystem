#include <svccall.h>
#include <csr.h>
#include <uart.h>
#include <printf.h>
#include <hart.h>
#include <sbicalls.h>


void svccall_handle(int hart){ //OS make sbi do things
    long *mscratch;
    unsigned long mip;
    CSR_READ(mscratch, "mscratch"); //Read what type of ecall brought us here
    //printf("Handled %d\n",mscratch[XREG_A7]);
    switch(mscratch[XREG_A7]){ 
        case SBI_PUT_CHAR: 
            uart_put(mscratch[XREG_A0]);
            break;
        case SBI_GET_CHAR:
            mscratch[XREG_A0] = uart_buffer_get();
            break;
        case SBI_WHOAMI:
            mscratch[XREG_A0] = hart;
            break;
        case SBI_HART_STATUS:
            mscratch[XREG_A0] = hart_get_status(mscratch[XREG_A0]);
            break;
        case SBI_HART_START:
            mscratch[XREG_A0] = hart_start(mscratch[XREG_A0], mscratch[XREG_A1], mscratch[XREG_A2]);
            break;
        case SBI_HART_STOP:
            mscratch[XREG_A0] = hart_stop(hart);
            break;
        case SBI_GET_TIME:
            mscratch[XREG_A0] = clint_get_time();
            break;
        case SBI_SET_TIMECMP:
            clint_set_mtimecmp(mscratch[XREG_A0], mscratch[XREG_A1]);
            break;
        case SBI_INTERRUPT_IN:
            clint_add_mtimecmp(mscratch[XREG_A0], mscratch[XREG_A1]);
            break;
        case SBI_CLEAR_TIMER_INTERRUPT:
            
            CSR_READ(mip, "mip");
            CSR_WRITE("mip", mip & ~SIP_STIP);
            break;
        case SBI_RTC_GET_TIME:
            mscratch[XREG_A0] = rtc_get_time();
            break;

        default:
            printf("Unknown system call %d on hart %d\n", mscratch[XREG_A7], hart);
    }
}