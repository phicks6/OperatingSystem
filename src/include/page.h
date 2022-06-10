#pragma once
#define PAGE_SIZE 4096

#include <stdint.h>

void page_init(void);
void page_print(void);
void *page_phalloc(void);
void *page_nphalloc(int count);
void page_free(void *pg);
uint64_t align_up(uint64_t addr);
uint64_t align_down(uint64_t addr);
