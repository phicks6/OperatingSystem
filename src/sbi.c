#include <sbi.h>
//Defines a number of system calls for the OS to get info or set thing in the SBI
//They all work in a similar way by putting the number of the ecall (as defined int sbi/include/sbicalls.h) in mscratch[XREG_A7]

void sbi_put_char(char c){
   asm volatile("mv a7, %0\nmv a0, %1\necall" :: "r"(SBI_PUT_CHAR), "r"(c) : "a0", "a7");
}

char sbi_get_char(void)
{
    char c;
    asm volatile ("mv a7, %1\necall\nmv %0, a0\n" : "=r"(c) : "r"(SBI_GET_CHAR) : "a7", "a0");
    return c;
}

int sbi_hart_get_status(int hart) {
	int stat;
	asm volatile ("mv a7, %1\nmv a0, %2\necall\nmv %0, a0\n" : "=r"(stat) : "r"(SBI_HART_STATUS), "r"(hart) : "a0", "a7");
	return stat;
}

int sbi_hart_start(unsigned int hart, unsigned long target, unsigned long scratch){
    int stat;
    asm volatile(
        "mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\necall\nmv %0, a0\n"
        : "=r"(stat)
        : "r"(SBI_HART_START), "r"(hart), "r"(target), "r"(scratch)
        : "a0", "a1", "a2", "a7");
    return stat;
}


int sbi_hart_stop(void) {
	int stat;
	asm volatile ("mv a7, %1\necall\nmv a0, %0" : "=r"(stat) :
				  "r"(SBI_HART_STOP) : "a0", "a7");
	return stat;
}

void sbi_poweroff(void) {
	asm volatile("mv a7, %0\necall" : : "r"(SBI_POWEROFF) : "a0", "a7");
}

unsigned long sbi_get_time(void) {
	unsigned long ret;
	asm volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(SBI_GET_TIME) : "a0", "a7");
	return ret;
}

void sbi_set_timecmp(unsigned int hart, unsigned long val) {
	asm volatile("mv a7, %0\nmv a0, %1\nmv a1, %2\necall" :: "r"(SBI_SET_TIMECMP),
	             "r"(hart), "r"(val) : "a0", "a1", "a7"
	);
}

unsigned long sbi_rtc_get_time(void) {
	unsigned long ret;
	asm volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(SBI_RTC_GET_TIME) : "a0","a7");
	return ret;
}

int sbi_whoami(void){
    int ret;
    asm volatile("mv a7, %1\necall\nmv %0, a0"
                 : "=r"(ret)
                 : "r"(SBI_WHOAMI)
                 : "a0", "a7");
    return ret;
}

void sbi_clear_timer_interrupt(void){
    asm volatile("mv a7, %0\necall" ::"r"(SBI_CLEAR_TIMER_INTERRUPT) : "a7");
}

void sbi_interrupt_in(unsigned int hart, unsigned long val){
    asm volatile(
        "mv a7, %0\nmv a0, %1\nmv a1, %2\necall" ::"r"(SBI_INTERRUPT_IN),
        "r"(hart), "r"(val)
        : "a0", "a1", "a7");
}
