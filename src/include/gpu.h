#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <pcie.h>
#include <virtio.h>

#define VIRTIO_GPU_EVENT_DISPLAY   1
typedef struct virtio_gpu_config{
   uint32_t  events_read;
   uint32_t  events_clear;
   uint32_t  num_scanouts;
   uint32_t  reserved;
}virtio_gpu_config;

#define VIRTIO_GPU_FLAG_FENCE  1
typedef struct gpu_control_header{
   uint32_t control_type;
   uint32_t flags;
   uint64_t fence_id;
   uint32_t context_id;
   uint32_t padding;
}gpu_control_header;

enum ControlType{
   /* 2D Commands */
   VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
   VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
   VIRTIO_GPU_CMD_RESOURCE_UNREF,
   VIRTIO_GPU_CMD_SET_SCANOUT,
   VIRTIO_GPU_CMD_RESOURCE_FLUSH,
   VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
   VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
   VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
   VIRTIO_GPU_CMD_GET_CAPSET_INFO,
   VIRTIO_GPU_CMD_GET_CAPSET,
   VIRTIO_GPU_CMD_GET_EDID,
   /* cursor commands */
   VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
   VIRTIO_GPU_CMD_MOVE_CURSOR,
   /* success responses */
   VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
   VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
   VIRTIO_GPU_RESP_OK_CAPSET_INFO,
   VIRTIO_GPU_RESP_OK_CAPSET,
   VIRTIO_GPU_RESP_OK_EDID,
   /* error responses */
   VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
   VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
   VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
   VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
   VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
   VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

#define VIRTIO_GPU_MAX_SCANOUTS 16
typedef struct Rectangle{
   uint32_t x;
   uint32_t y;
   uint32_t width;
   uint32_t height;
}Rectangle;

typedef struct gpu_display_info_response{
   gpu_control_header hdr;  /* VIRTIO_GPU_RESP_OK_DISPLAY_INFO */
   struct GpuDisplay {
       struct Rectangle rect;
       uint32_t enabled;
       uint32_t flags;
   } displays[VIRTIO_GPU_MAX_SCANOUTS];
}gpu_display_info_response;

enum GpuFormats{
   B8G8R8A8_UNORM = 1,
   B8G8R8X8_UNORM = 2,
   A8R8G8B8_UNORM = 3,
   X8R8G8B8_UNORM = 4,
   R8G8B8A8_UNORM = 67,
   X8B8G8R8_UNORM = 68,
   A8B8G8R8_UNORM = 121,
   R8G8B8X8_UNORM = 134,
};

typedef struct gpu_resource_create2d_request{
   gpu_control_header hdr; /* VIRTIO_GPU_CMD_RESOURCE_CREATE_2D */
   uint32_t resource_id;   /* We get to give a unique ID to each resource */
   uint32_t format;        /* From GpuFormats above */
   uint32_t width;
   uint32_t height;
}gpu_resource_create2d_request;

typedef struct ResourceUnrefRequest{
    gpu_control_header hdr; /* VIRTIO_GPU_CMD_RESOURCE_UNREF */
    uint32_t resource_id;
    uint32_t padding;
}ResourceUnrefRequest;

/* VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING */
typedef struct gpu_resource_attach_backing{
	gpu_control_header hdr;
	uint32_t resource_id;
	uint32_t nr_entries;
}gpu_resource_attach_backing;

typedef struct gpu_mem_entry{
   uint64_t addr;
   uint32_t length;
   uint32_t padding;
}gpu_mem_entry;


typedef struct gpu_set_scanout_request{
    gpu_control_header hdr; /* VIRTIO_GPU_CMD_SET_SCANOUT */
    Rectangle rect;
    uint32_t scanout_id;
    uint32_t resource_id;
}gpu_set_scanout_request;

typedef struct gpu_transfer_to_host_2d{
   gpu_control_header hdr;
   Rectangle rect;
   uint64_t offset;
   uint32_t resource_id;
   uint32_t padding;
}gpu_transfer_to_host_2d;

typedef struct gpu_resource_flush{
   struct gpu_control_header hdr;
   Rectangle rect;
   uint32_t resource_id;
   uint32_t padding;
}gpu_resource_flush;

typedef struct Pixel{
    /* This pixel structure must match the format! */
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
}Pixel;


typedef struct GPUDevice{
    volatile struct PcieDriver * driver;
    volatile struct VirtioCapInfo * vinfo;
    volatile struct virtio_gpu_config * type4;
    volatile struct Virtq * vqueue;
    uint32_t width;
    uint32_t height;
    Pixel * frameBuffer;
    uint16_t index;
    bool waiting_on_interrupt;
    bool waiting_on_config_interrupt;
}GPUDevice;

bool gpu_probe(uint8_t bus, uint8_t slot, uint8_t function);

GPUDevice * gpu_create(void);



void gpu_request(void *buffer, uint8_t *status, uint16_t start_sector, uint16_t size, uint8_t direction);
void gpu_get_dim(void);
void gpu_create_resource(void);
void gpu_attach_resource(void);
void gpu_set_scanout(void);
void gpu_transfer_buffer(Rectangle *rect);
void gpu_flush_buffer(void);
void gpu_test(void);
bool gpu_claim_irq(int irq);

void gpu_init(void);
void gpu_update_frame(void);

void fill_rect(uint32_t screen_width, uint32_t screen_height, Pixel *buffer, Rectangle *rect, Pixel *fill_color);
void sys_change_pixel(int x, int y, Pixel color);
//void copy_to_frame_buffer(uint32_t startX, uint32_t startY, void *buffer)
Pixel *get_frame_buffer(int *width, int *height);