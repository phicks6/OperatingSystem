#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <pcie.h>
#include <virtio.h>

typedef enum virtio_input_config_select {
    VIRTIO_INPUT_CFG_UNSET = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO = 0x12,
} virtio_input_config_select;

typedef struct virtio_input_absinfo {
    uint32_t min;
    uint32_t max;
    uint32_t fuzz;
    uint32_t flat;
    uint32_t res;
} virtio_input_absinfo;
typedef struct virtio_input_devids {
    uint16_t bustype;
    uint16_t vendor;
    uint16_t product;
    uint16_t version;
} virtio_input_devids;

typedef volatile struct virtio_input_config {
    uint8_t select;
    uint8_t subsel;
    uint8_t size;
    uint8_t reserved[5];
    union {
        char string[128];
        uint8_t bitmap[128];
        struct virtio_input_absinfo abs;
        struct virtio_input_devids ids;
    };
} virtio_input_config;


typedef struct input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} input_event;

typedef enum RingBufferBehavior {
    RB_OVERWRITE,  // Overwrite old buffer data
    RB_DISCARD,    // Silently discard new buffer data
    RB_ERROR       // Don't do anything and return an error.
} RingBufferBehavior;

typedef struct RingBuffer{
    uint32_t pos;            // The starting index of the data
    uint32_t num_elements;  // The number of *valid* elements in the ring
    uint32_t element_size;
    uint32_t capacity;      // The size of the ring buffer
    uint32_t behavior;      // What should be done if the ring buffer is full?
    void * buffer;        // The actual ring buffer in memory
}RingBuffer;


bool ring_push(RingBuffer *buf, void *event);
bool ring_pop(RingBuffer *buf, void *event);

typedef struct InputDevice{
    volatile struct PcieDriver * driver;
    volatile struct VirtioCapInfo * vinfo;
    volatile struct virtio_input_config * type4;
    volatile struct Virtq * vqueue;
    volatile input_event * event_buffer;
    uint16_t index;
    uint16_t qsize;
    bool waiting_on_interrupt;
}InputDevice;

typedef struct Pos{
    uint32_t x;
    uint32_t y;
}Pos;

bool input_probe(uint8_t bus, uint8_t slot, uint8_t function);

InputDevice * input_create(void);

uint8_t input_read(void *buffer, uint64_t offset, uint64_t size);

bool input_claim_irq(int irq);

void input_init(void);

void input_copy_to_internal(int interupt_index);
void vomit_inputs(void);

bool get_tablet_buffer_event(input_event *event);
void track_mouse(void);
int get_mouse_x(void);
int get_mouse_y(void);
int get_mouse_state(void);
