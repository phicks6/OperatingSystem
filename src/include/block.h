#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <pcie.h>
#include <virtio.h>

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_T_DISCARD 11
#define VIRTIO_BLK_T_WRITE_ZEROES 13


typedef struct virtio_blk_config{
   uint64_t capacity;
   uint32_t size_max;
   uint32_t seg_max;
   struct virtio_blk_geometry{
      uint16_t cylinders;
      uint8_t heads;
      uint8_t sectors;
   }geometry;
   uint32_t blk_size; // the size of a sector, usually 512
   struct virtio_blk_topology {
      // # of logical blocks per physical block (log2)
      uint8_t physical_block_exp;
      // offset of first aligned logical block
      uint8_t alignment_offset;
      // suggested minimum I/O size in blocks
      uint16_t min_io_size;
      // optimal (suggested maximum) I/O size in blocks
      uint32_t opt_io_size;
   } topology;
   uint8_t writeback;
   uint8_t unused0[3];
   uint32_t max_discard_sectors;
   uint32_t max_discard_seg;
   uint32_t discard_sector_alignment;
   uint32_t max_write_zeroes_sectors;
   uint32_t max_write_zeroes_seg;
   uint8_t write_zeroes_may_unmap;
   uint8_t unused1[3];
} virtio_blk_config;

typedef struct BlockDevice{
    volatile struct PcieDriver * driver;
    volatile struct VirtioCapInfo * vinfo;
    volatile struct virtio_blk_config * type4;
    volatile struct Virtq * vqueue;
    uint16_t index;
    uint32_t blk_size;
    uint16_t qsize;
    bool *waiting_on_interrupt;
}BlockDevice;

typedef struct BlockHeader{
   uint32_t type;      // IN/OUT
   uint32_t reserved;
   uint64_t sector;
} BlockHeader;


bool block_probe(uint8_t bus, uint8_t slot, uint8_t function);

BlockDevice * block_create(void);

uint8_t block_read(void *buffer, uint64_t offset, uint64_t size);
uint8_t block_write(void *buffer, uint64_t offset, uint64_t size);

void block_request(void *buffer, uint8_t *status, uint16_t start_sector, uint16_t size, uint8_t direction);

bool block_claim_irq(int irq);

void block_init(void);