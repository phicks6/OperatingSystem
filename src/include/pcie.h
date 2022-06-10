#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <printf.h>
#include <rng.h>

#define VENDER_SPECIFIC 9
#define PCIE 16
#define MMIO_ECAM_BASE 0x30000000



struct Capability{
    uint8_t id;
    uint8_t next;
};



typedef struct PcieDriver{
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    bool (*probe)(uint8_t bus, uint8_t slot, uint8_t function);
    bool (*claim_irq)(int irq);
    void (*init)();
    
} PcieDriver;

typedef struct DriverList{
    uint16_t vendor_id;
    uint16_t device_id;
    PcieDriver * driver;
    uint64_t next;
} DriverList;

typedef struct VirtioCapInfo{
    volatile struct virtio_pci_common_cfg * type1;
    volatile struct virtio_pci_notify_cap * type2;
    volatile struct virtio_pci_isr_cap * type3;
} VirtioCapInfo;

typedef struct Virtq{
    volatile struct virtq_desc * desc;
    volatile struct virtq_queue_driver * queue_driver;
    volatile struct virtq_queue_device * queue_device;
} Virtq;

struct EcamHeader{
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command_reg;
    uint16_t status_reg;
    uint8_t revision_id;
    uint8_t prog_if;
    union {
        uint16_t class_code;
        struct {
            uint8_t class_subcode;
            uint8_t class_basecode;
        };
    };
    uint8_t cacheline_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    union {
        struct {
            uint32_t bar[6];
            uint32_t cardbus_cis_pointer;
            uint16_t sub_vendor_id;
            uint16_t sub_device_id;
            uint32_t expansion_rom_addr;
            uint8_t  capes_pointer;
            uint8_t  reserved0[3];
            uint32_t reserved1;
            uint8_t  interrupt_line;
            uint8_t  interrupt_pin;
            uint8_t  min_gnt;
            uint8_t  max_lat;
        } type0;
        struct {
            uint32_t bar[2];
            uint8_t  primary_bus_no;
            uint8_t  secondary_bus_no;
            uint8_t  subordinate_bus_no;
            uint8_t  secondary_latency_timer;
            uint8_t  io_base;
            uint8_t  io_limit;
            uint16_t secondary_status;
            uint16_t memory_base;
            uint16_t memory_limit;
            uint16_t prefetch_memory_base;
            uint16_t prefetch_memory_limit;
            uint32_t prefetch_base_upper;
            uint32_t prefetch_limit_upper;
            uint16_t io_base_upper;
            uint16_t io_limit_upper;
            uint8_t  capes_pointer;
            uint8_t  reserved0[3];
            uint32_t expansion_rom_addr;
            uint8_t  interrupt_line;
            uint8_t  interrupt_pin;
            uint16_t bridge_control;
        } type1;
        struct {
            uint32_t reserved0[9];
            uint8_t  capes_pointer;
            uint8_t  reserved1[3];
            uint32_t reserved2;
            uint8_t  interrupt_line;
            uint8_t  interrupt_pin;
            uint8_t  reserved3[2];
        } common;
    };
};



void init_pcie(int uh);

static volatile struct EcamHeader *pcie_get_ecam(uint8_t bus, uint8_t device, uint8_t function, uint16_t reg){
    uint64_t bus64 = bus & 0b11111111;
    uint64_t device64 = device & 0b11111;
    uint64_t function64 = function & 0b1111111;
    uint64_t reg64 = reg & 0b1111111111;

    volatile struct EcamHeader * ecam;
    ecam = MMIO_ECAM_BASE | (bus64 << 20) | (device64 << 15) | (function64 << 12) | (reg64 << 2);

    return ecam;
}

static void pcie_setup_device(volatile struct EcamHeader *ecam, uint8_t bus, uint8_t slot);
static void pcie_setup_bridge(volatile struct EcamHeader *ecam, uint8_t bus);
void pcie_register_driver(uint16_t vendor_id, uint16_t device_id, PcieDriver *driver);
PcieDriver * pcie_find_driver(uint16_t vendor_id, uint16_t device_id);
bool pcie_claim_irq(int irq);
static void pcie_enumerate_capabilities(volatile struct EcamHeader *ecam);

uint64_t pcie_read_bar(volatile struct EcamHeader *ecam, uint8_t bar, uint8_t bar_size);

