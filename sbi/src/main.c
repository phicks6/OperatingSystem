 #include <uart.h>
#include <printf.h>
#include <plic.h>
#include <csr.h>
#include <lock.h>
#include <hart.h>

//Lets plic use uart interupts for input handling
void plic_init(){
    plic_enable(0,10); //have hart 0 enable 10/UART
    plic_set_priority(10,6); //Give UART a priority of 6
    plic_set_threshold(0,0); //Have hart 0 see all above 0
}

static void rtc_init() {
	(void)rtc_get_time();
}

static void pmp_init(){
    // Top of range address is 0xffff_ffff. Since this is pmpaddr0
    // then the bottom of the range is 0x0.
    CSR_WRITE("pmpaddr0", 0xffffffff >> 2);
    // A = 01 (TOR), X=1, W=1, and R=1
    CSR_WRITE("pmpcfg0", (0b01 << 3) | (1 << 2) | (1 << 1) | (1 << 0));
}

// in sbi/asm/clearbss.S
void clear_bss();

// in sbi/asm/start.S
void park();

// in sbi/asm/trap_handler.S
void sbi_trap_vector();

long int SBI_GPREGS[8][32];
struct Mutex hart0InitializationLock = {MUTEX_LOCKED};


ATTR_NAKED_NORET
int main(int hart){
    //printf("Hart %d booting\n",hart);
    if(hart==0){ //Root hart does extra in the booting up and makes it the only one handling the 
        //After clearing the bss and initializing the uart the other harts can begin their setup
        clear_bss();
        uart_init();
        
        // Let the other HARTs go
        mutex_unlock(&hart0InitializationLock);
        plic_init();
        pmp_init();
        rtc_init();
        
        sbi_hart_data[0].status = HS_STARTED;
        sbi_hart_data[0].target_address = 0;
        sbi_hart_data[0].priv_mode = HPM_MACHINE;
        
        CSR_WRITE("mscratch", &SBI_GPREGS[hart][0]); //Store trap frame into mscratch register
        CSR_WRITE("sscratch", hart);//Store hart # in sscratch so the OS can read it.
        
        CSR_WRITE("mepc", OS_LOAD_ADDR); //sets where we load to after MRET being our OS's start.S
        CSR_WRITE("mtvec", sbi_trap_vector); //sets were we jump to when we context switch, in trap_handler.S
        CSR_WRITE("mie", MIE_MEIE | MIE_MTIE | MIE_MSIE); //Enable external interupts | timer interupts | software interupts
        CSR_WRITE("mideleg", SIP_SEIP | SIP_STIP | SIP_SSIP); //Delegates async interupts to supervisor mode
        CSR_WRITE("medeleg", MEDELEG_ALL);
        CSR_WRITE("mstatus", MSTATUS_FS_INITIAL | MSTATUS_MPP_SUPERVISOR | MSTATUS_MPIE);

        
        MRET();

    }else{ //All other harts
        mutex_spinlock(&hart0InitializationLock); //Wait until hart 0 is done with its set up
        mutex_unlock(&hart0InitializationLock); //Let go of lock because we only want to wait on hart 0
		pmp_init();

        //Other harts start as stopped and will need to be manualy started later 
		sbi_hart_data[hart].status = HS_STOPPED;
		sbi_hart_data[hart].target_address = 0;
		sbi_hart_data[hart].priv_mode = HPM_MACHINE;

		CSR_WRITE("mscratch", &SBI_GPREGS[hart][0]);//Store trap frame into mscratch register
		CSR_WRITE("sscratch", hart); //Store hart # in sscratch so the OS can read it.

		CSR_WRITE("mepc", park); //After they are done setting up go park

		//These two are 0 so that only one hart (hart 0) handles them, this makes things simpler
        CSR_WRITE("mideleg", 0); 
		CSR_WRITE("medeleg", 0);

		CSR_WRITE("mie", MIE_MSIE); //Still need software interupts so the root hart can be awoken with an interupt
		CSR_WRITE("mstatus", MSTATUS_FS_INITIAL | MSTATUS_MPP_MACHINE | MSTATUS_MPIE);
		CSR_WRITE("mtvec", sbi_trap_vector);//sets were we jump to when we context switch, in trap_handler.S
		MRET();
    }
}
