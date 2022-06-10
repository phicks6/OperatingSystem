#pragma once

#include <stdint.h>

#define PLIC_BASE           0x0c000000UL
#define PLIC_PRIORITY_BASE  0x4
#define PLIC_PENDING_BASE   0x1000
#define PLIC_ENABLE_BASE    0x2000
#define PLIC_ENABLE_STRIDE  0x80
#define PLIC_CONTEXT_BASE   0x200000
#define PLIC_CONTEXT_STRIDE 0x1000

#define PLIC_MODE_MACHINE    0x0
#define PLIC_MODE_SUPERVISOR 0x1

#define PLIC_PRIORITY(interrupt) \
    (PLIC_BASE + PLIC_PRIORITY_BASE * interrupt)

#define PLIC_THRESHOLD(hart, mode) \
    (PLIC_BASE + PLIC_CONTEXT_BASE + PLIC_CONTEXT_STRIDE * (2 * hart + mode))

#define PLIC_CLAIM(hart, mode) \
    (PLIC_THRESHOLD(hart, mode) + 4)

#define PLIC_ENABLE(hart, mode) \
    (PLIC_BASE + PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * (2 * hart + mode))

void plic_set_priority(int interrupt_id, char priority);

void plic_set_threshold(int hart, char priority);

void plic_enable(int hart, int interrupt_id);

void plic_disable(int hart, int interrupt_id);

uint32_t plic_claim(int hart);

void plic_complete(int hart, int id);

void plic_handle_irq(int id);