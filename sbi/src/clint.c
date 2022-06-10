#include <clint.h>

void clint_set_msip(unsigned int hart){ //Causes a hart to trap which takes us into trap.c
    if(hart >= 8){
        return;
    } 

    unsigned int *clint = (unsigned int *)CLINT_BASE_ADDRESS;
    clint[hart] = 1;
}

//msip is level triggered so we need to be able to clear it
void clint_clear_msip(unsigned int hart){
    if(hart >= 8){
        return;
    } 

    unsigned int *clint = (unsigned int *)CLINT_BASE_ADDRESS;
    clint[hart] = 0;
}


void clint_set_mtimecmp(unsigned int hart, unsigned long val){
    if(hart >= 8){
        return;
    } 
    unsigned long *mTimeCmpRegister = (unsigned long *)(CLINT_BASE_ADDRESS + CLINT_MTIMECMP_OFFSET);
    mTimeCmpRegister[hart] = val;
}

//Sets time val into the future
void clint_add_mtimecmp(unsigned int hart, unsigned long val){
    //printf("Setting timer ahead\n");
    clint_set_mtimecmp(hart, clint_get_time() + val);
}

unsigned long clint_get_time(){
    unsigned long tm;
    asm volatile("rdtime %0" : "=r"(tm));
    return tm;
}
