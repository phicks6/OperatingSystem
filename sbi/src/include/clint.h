#pragma once

#define CLINT_BASE_ADDRESS 0x2000000
#define CLINT_MTIMECMP_OFFSET 0x4000

#define CLINT_MTIMECMP_INFINITE  0x7FFFFFFFFFFFFFFFUL

void clint_set_msip(unsigned int hart);
void clint_clear_msip(unsigned int hart);
void clint_set_mtimecmp(unsigned int hart, unsigned long val);
void clint_add_mtimecmp(unsigned int hart, unsigned long val);
unsigned long clint_get_time();
