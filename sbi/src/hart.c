#include <hart.h>
#include <lock.h>
#include <csr.h>
#include <clint.h>

//Defines the hart table
HartData sbi_hart_data[8];

struct Mutex sbi_hart_lock[8];

void park();

//if the hart exists return its status from the table
HartStatus hart_get_status(unsigned int hart){
    if(hart >= 8){
        return HS_INVALID;
    }
    return sbi_hart_data[hart].status;
}


bool hart_start(unsigned int hart, unsigned long target, unsigned long scratch){
    //printf("hart_start hart %d scratch is %lx\n",hart, scratch);
    bool ret = true;
    if (!mutex_trylock(&sbi_hart_lock[hart])) {
        return false;
    }

    if (sbi_hart_data[hart].status != HS_STOPPED) {
        ret = false;
    }
    else {
        sbi_hart_data[hart].status = HS_STARTING;
        sbi_hart_data[hart].target_address = target;
        sbi_hart_data[hart].scratch = scratch;
        
        clint_set_msip(hart);
    }
    mutex_unlock(&sbi_hart_lock[hart]);
    return ret;
}

bool hart_stop(unsigned int hart) {
    if (!mutex_trylock(&sbi_hart_lock[hart])){
        return false;
    }
    if(sbi_hart_data[hart].status != HS_STARTED){
        return false;
    }else{
        sbi_hart_data[hart].status = HS_STOPPED;
        CSR_WRITE("mepc", park); //Stopped harts are parked harts
        CSR_WRITE("mstatus", MSTATUS_MPP_MACHINE | MSTATUS_MPIE);

        //Prevents a stopped hart from hearing all the interupts a started hart can
        CSR_WRITE("mie", MIE_MSIE);
        mutex_unlock(&sbi_hart_lock[hart]);
        MRET();
    }

    mutex_unlock(&sbi_hart_lock[hart]);
    return false;
}

void hart_handle_msip(unsigned int hart) {
    mutex_spinlock(&sbi_hart_lock[hart]);
    //printf("Handling msip\n");
    clint_clear_msip(hart);

    if (sbi_hart_data[hart].status == HS_STARTING) {
        CSR_WRITE("mepc", sbi_hart_data[hart].target_address); //this is what the hart will run after starting up
        //printf("writing sbi_hart_data[hart].target_address %lx into mepc\n",sbi_hart_data[hart].target_address);
        CSR_WRITE("mstatus", MSTATUS_MPP_SUPERVISOR | MSTATUS_MPIE | MSTATUS_FS_INITIAL);
        
        //Open up a running hart to more interupts
        CSR_WRITE("mie", MIE_MEIE | MIE_SSIE | MIE_STIE | MIE_MTIE); 
        CSR_WRITE("mideleg", SIP_SEIP | SIP_STIP | SIP_SSIP);
        CSR_WRITE("medeleg", MEDELEG_ALL);
        
        //printf("writing sbi_hart_data[hart].scratch %lx into sscratch\n",sbi_hart_data[hart].scratch);
        CSR_WRITE("sscratch", sbi_hart_data[hart].scratch);
        
        sbi_hart_data[hart].status = HS_STARTED;
    }

    mutex_unlock(&sbi_hart_lock[hart]);
    MRET();
}

