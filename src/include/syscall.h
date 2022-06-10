#pragma once

#include <stdbool.h>
#include <stdint.h>

#define EXIT 0
#define PUT_CHAR 1
#define GET_CHAR 2

void syscall_handle(int hart, uint64_t sepc, int64_t *scratch);