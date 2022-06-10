#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pcie.h>
#include <rng.h>

#define VIRTIO_VENDER 0x1af4
#define ENTROPY_DEVICE 0x1044
#define BLOCK_DEVICE 0x1042
#define GPU_DEVICE 0x1050
#define INPUT_DEVICE 0x1052
         /* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
        /* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
        /* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG 3
        /* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
        /* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG 5

typedef struct virtio_pci_cap{
   uint8_t cap_vndr;   /* Generic PCI field: PCI_CAP_ID_VNDR */
   uint8_t cap_next;   /* Generic PCI field: next ptr. */
   uint8_t cap_len;    /* Generic PCI field: capability length */
   uint8_t cfg_type;   /* Identifies the structure. */
   uint8_t bar;        /* Which BAR to find it. */
   uint8_t padding[3]; /* Pad to full dword. */
   uint32_t offset;    /* Offset within bar. */
   uint32_t length;    /* Length of the structure, in bytes. */
} virtio_pci_cap;

typedef struct virtio_pci_common_cfg{ //TYPE 1
   uint32_t device_feature_select; /* read-write */
   uint32_t device_feature; /* read-only for driver */
   uint32_t driver_feature_select; /* read-write */
   uint32_t driver_feature; /* read-write */
   uint16_t msix_config; /* read-write */
   uint16_t num_queues; /* read-only for driver */
   uint8_t device_status; /* read-write */
   uint8_t config_generation; /* read-only for driver */
   /* About a specific virtqueue. */
   uint16_t queue_select; /* read-write */
   uint16_t queue_size; /* read-write */
   uint16_t queue_msix_vector; /* read-write */
   uint16_t queue_enable; /* read-write */
   uint16_t queue_notify_off; /* read-only for driver */
   uint64_t queue_desc; /* read-write */
   uint64_t queue_driver; /* read-write */
   uint64_t queue_device; /* read-write */
} virtio_pci_common_cfg;

typedef struct virtio_pci_notify_cap{ //TYPE 2
   struct virtio_pci_cap cap;
   uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} virtio_pci_notify_cap;


#define BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier)   ((offset) + (queue_notify_off) * (notify_off_multiplier))

typedef struct virtio_pci_isr_cap{ //TYPE 3
    union {
       struct {
          unsigned queue_interrupt: 1;
          unsigned device_cfg_interrupt: 1;
          unsigned reserved: 30;
       };
       unsigned int isr_cap;
    };
} virtio_pci_isr_cap;





typedef struct virtq_desc {
   /* Address (guest-physical). */
   uint64_t addr;
   /* Length. */
   uint32_t len;
   /* This marks a buffer as continuing via the next field. */
   #define VIRTQ_DESC_F_NEXT   1
   /* This marks a buffer as device write-only (otherwise device read-only). */
   #define VIRTQ_DESC_F_WRITE     2
   /* This means the buffer contains a list of buffer descriptors. */
   #define VIRTQ_DESC_F_INDIRECT   4
   /* The flags as indicated above. */
   uint16_t flags;
   /* Next field if flags & NEXT */
   uint16_t next;
} virtq_desc;

typedef volatile struct virtq_queue_driver {
   #define VIRTQ_AVAIL_F_NO_INTERRUPT      1
   uint16_t flags;
   uint16_t idx;
   uint16_t ring[];
   //uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
} virtq_queue_driver;

/* le32 is used here for ids for padding reasons. */
typedef volatile struct virtq_used_elem {
   /* Index of start of used descriptor chain. */
   uint32_t id;
   /* Total length of the descriptor chain which was used (written to) */
   uint32_t len;
} virtq_used_elem;

typedef volatile struct virtq_queue_device {
   #define VIRTQ_USED_F_NO_NOTIFY  1
   uint16_t flags;
   uint16_t idx;
   struct virtq_used_elem ring[];
   //uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
} virtq_queue_device;



void virtio_init(void);
void virtio_notify(uint64_t barAddress,uint64_t offset ,uint64_t queue_notify_off,uint64_t notify_off_multiplier);
void virtio_set_type1(uint16_t device_id, virtio_pci_common_cfg *type1);
void virtio_set_type2(uint16_t device_id, virtio_pci_notify_cap *type2);
void virtio_set_type3(uint16_t device_id, virtio_pci_isr_cap *type3);
void virtio_set_type4(uint16_t device_id, void* type4);
