#pragma once
#include "../../sbi/src/include/sbicalls.h"


void sbi_put_char(char c);
char sbi_get_char(void);
int sbi_hart_get_status(int hart);

int sbi_hart_start(unsigned int hart, unsigned long target, unsigned long scratch);
int sbi_hart_stop(void);
void sbi_poweroff(void);
unsigned long sbi_get_time(void);
void sbi_set_timecmp(unsigned int hart, unsigned long val);
unsigned long sbi_rtc_get_time(void);
int sbi_whoami(void);
void sbi_clear_timer_interrupt(void);
void sbi_interrupt_in(unsigned int hart, unsigned long val);