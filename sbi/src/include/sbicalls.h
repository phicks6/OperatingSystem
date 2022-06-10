#pragma once
//We get to define what the ecall numbers means 
#define SBI_PUT_CHAR 1
#define SBI_GET_CHAR 2

#define SBI_WHOAMI 3

#define SBI_HART_STATUS 5
#define SBI_HART_START 6
#define SBI_HART_STOP 7

#define SBI_GET_TIME     10
#define SBI_SET_TIMECMP  11
#define SBI_INTERRUPT_IN 12
#define SBI_CLEAR_TIMER_INTERRUPT 13
#define SBI_RTC_GET_TIME 14

#define SBI_POWEROFF 215