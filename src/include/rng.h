#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <pcie.h>
#include <virtio.h>

typedef struct RNGDevice{
    volatile struct PcieDriver * driver;
    volatile struct VirtioCapInfo * vinfo;
    volatile struct Virtq * vqueue;
    uint16_t index;
    bool waiting_on_interrupt;
}RNGDevice;

bool rng_probe(uint8_t bus, uint8_t slot, uint8_t function);

RNGDevice * rng_create(void);

bool rng_fill(void *buffer, uint16_t size);


bool rng_claim_irq(int irq);

void rng_init(void);



