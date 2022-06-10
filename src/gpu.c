#include <gpu.h>
#include <pcie.h>
#include <virtio.h>
#include <mmu.h>
#include <csr.h>
#include <util.h>


GPUDevice * gpu_device;

gpu_display_info_response * test;

bool gpu_probe(uint8_t bus, uint8_t slot, uint8_t function){
    gpu_device->driver->bus = bus;
    gpu_device->driver->slot = slot;
    return true;
}

GPUDevice * gpu_create(){
    gpu_device = pzalloc(sizeof(GPUDevice));
    gpu_device->driver = pzalloc(sizeof(PcieDriver));
    gpu_device->vqueue = pzalloc(sizeof(Virtq));
    gpu_device->vinfo = pzalloc(sizeof(VirtioCapInfo));

    gpu_device->driver->vendor_id = VIRTIO_VENDER;
    gpu_device->driver->device_id = GPU_DEVICE;
    gpu_device->driver->probe = gpu_probe;
    gpu_device->driver->claim_irq = gpu_claim_irq;
    gpu_device->driver->init = gpu_init;
    gpu_device->index = 0;
    return gpu_device;
}

//Claims a irq
bool gpu_claim_irq(int irq){
    if(32 + ((gpu_device->driver->bus + gpu_device->driver->slot)%4) == irq){
        uint32_t interuptThing =  *(uint32_t*)gpu_device->vinfo->type3;
        virtio_pci_isr_cap *temp = &interuptThing;
        if(temp->device_cfg_interrupt){
            gpu_device->waiting_on_config_interrupt = 0;
            //gpu_get_dim();
            return true;
        }
        if(temp->queue_interrupt){
            //printf("gpu claimed an interupt\n");
            gpu_device->waiting_on_interrupt = 0;
            return true;
        }
    }else{

    }

    return false;
}



void gpu_init(){
    gpu_device->waiting_on_config_interrupt = 1;


    uint16_t preferedQsize = 64;
    
    //Reset device
    gpu_device->vinfo->type1->device_status = 0;
    //Set acknowledge status bit
    gpu_device->vinfo->type1->device_status = 1;
    //Set driver status bit
    gpu_device->vinfo->type1->device_status |= 2;
    //Set features_ok status bit
    gpu_device->vinfo->type1->device_status |= 8;

    //Select which queue we are working with
    gpu_device->vinfo->type1->queue_select = 0;
    
    //Negotiate with device
    //printf("original Qsize = 0x%lx\n",gpu_device->vinfo->type1->queue_size);
    gpu_device->vinfo->type1->queue_size = preferedQsize;
    uint16_t queueSize = gpu_device->vinfo->type1->queue_size;
    //printf("negotiated Qsize = 0x%lx\n",gpu_device->vinfo->type1->queue_size);


    //Set up memory for the descriptor table, driver ring and device ring 
    uint64_t virtAdd;
    virtAdd = (uint64_t)pzalloc(16 * queueSize); //Continuous
    gpu_device->vqueue->desc = (struct virtq_desc *)virtAdd;
    gpu_device->vinfo->type1->queue_desc = mmu_translate(kernel_mmu_table, virtAdd);

    virtAdd = (uint64_t)pzalloc(6 + 2 * queueSize); //Continuous
    gpu_device->vqueue->queue_driver = (struct virtq_queue_driver *)virtAdd;
    gpu_device->vinfo->type1->queue_driver = mmu_translate(kernel_mmu_table, virtAdd);

    virtAdd = (uint64_t)pzalloc(6 + 8 * queueSize); //Continuous
    gpu_device->vqueue->queue_device = (struct virtq_queue_device *)virtAdd;
    gpu_device->vinfo->type1->queue_device = mmu_translate(kernel_mmu_table, virtAdd);
    
    //Enable the queue 
    gpu_device->vinfo->type1->queue_enable = 1;
    
    //Set device to live with the driver_ok bit which apparently gets cleared by writing the memory space bit of the ecam's command register
    gpu_device->vinfo->type1->device_status |= 4;

    //Gets dimensions,creates a frame buffer, attaches that frame buffer, and selects a screen
    gpu_get_dim();
    gpu_create_resource();
    gpu_attach_resource();
    gpu_set_scanout();

    //Fills screen with a grey
    Rectangle r1 = {0, 0, gpu_device->width, gpu_device->height};
    Pixel z1 = { 88, 89, 91, 255 };
    fill_rect(gpu_device->width, gpu_device->height, gpu_device->frameBuffer, &r1, &z1);
    
    //Tell gpu to update it's frame buffer
    gpu_transfer_buffer(&r1);
    gpu_flush_buffer();

}

//Gathers all the pieces to piece together the address to notify the device 
void gpu_notify(void){
    uint64_t barAddress;
    uint64_t offset;
    uint64_t queue_notify_off;
    uint64_t notify_off_multiplier;

    volatile struct EcamHeader* ecam = pcie_get_ecam(gpu_device->driver->bus, gpu_device->driver->slot, gpu_device->driver->function, 0);
    barAddress = pcie_read_bar(ecam,gpu_device->vinfo->type2->cap.bar,64);
    offset = gpu_device->vinfo->type2->cap.offset;
    queue_notify_off = gpu_device->vinfo->type1->queue_notify_off; 
    notify_off_multiplier = gpu_device->vinfo->type2->notify_off_multiplier;
    
    virtio_notify(barAddress,offset,queue_notify_off,notify_off_multiplier);
}

void gpu_get_dim(){
    uint64_t headerAddress;
    uint64_t displayAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = gpu_device->index;
    mod = gpu_device->vinfo->type1->queue_size;


    gpu_control_header * header = pzalloc(sizeof(gpu_control_header));
    gpu_display_info_response * display_info = pzalloc(sizeof(gpu_display_info_response));
    header->control_type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    
    //Virtio uses physical memory.
    headerAddress = mmu_translate(kernel_mmu_table, (uint64_t)header);
    displayAddress = mmu_translate(kernel_mmu_table, (uint64_t)display_info);
    
    //Fill out descriptor
    gpu_device->vqueue->desc[at_idx].addr = headerAddress;
    gpu_device->vqueue->desc[at_idx].len = sizeof(gpu_control_header);
    gpu_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;

    gpu_device->vqueue->desc[(at_idx+1) % mod].addr = displayAddress;
    gpu_device->vqueue->desc[(at_idx+1) % mod].len = sizeof(gpu_display_info_response);
    gpu_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_WRITE;
    gpu_device->vqueue->desc[(at_idx+1) % mod].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    gpu_device->vqueue->queue_driver->ring[gpu_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    gpu_device->vqueue->queue_driver->idx += 1;

    //Update our index
    gpu_device->index = (gpu_device->index + 2) % mod;
    gpu_device->waiting_on_interrupt = 1;
    gpu_device->waiting_on_config_interrupt = 0;


    //Notify the device that it has work to do
    gpu_notify();
    //printf("Currently gpu_device->waiting_on_interrupt: %d and gpu_device->waiting_on_config_interrupt %d\n",gpu_device->waiting_on_interrupt,gpu_device->waiting_on_config_interrupt);
    while(gpu_device->waiting_on_interrupt || gpu_device->waiting_on_config_interrupt){
        if(gpu_device->waiting_on_interrupt){
            //printf("still waiting on interrupt\n");
        }
    } 
    gpu_device->width = display_info->displays[0].rect.width;
    gpu_device->height = display_info->displays[0].rect.height;
    //printf("GPU dimensions are %d by %d\n",gpu_device->width ,gpu_device->height);
    gpu_device->frameBuffer = pzalloc(sizeof(Pixel) * gpu_device->width * gpu_device->height);
}

//Let gpu know what formate the frame buffer will be in
void gpu_create_resource(){
    //printf("gpu_create_resource\n");
    uint64_t commandAddress;
    uint64_t displayAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = gpu_device->index;
    mod = gpu_device->vinfo->type1->queue_size;


    gpu_resource_create2d_request * create2d = pzalloc(sizeof(gpu_resource_create2d_request));
    gpu_display_info_response * display_info = pzalloc(sizeof(gpu_display_info_response));
    create2d->hdr.control_type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    create2d->format = R8G8B8A8_UNORM;
    create2d->height = gpu_device->height;
    create2d->width = gpu_device->width;
    create2d->resource_id = 1;
    
    //Virtio uses physical memory.
    commandAddress = mmu_translate(kernel_mmu_table, (uint64_t)create2d);
    displayAddress = mmu_translate(kernel_mmu_table, (uint64_t)display_info);
    
    //Fill out descriptor
    gpu_device->vqueue->desc[at_idx].addr = commandAddress;
    gpu_device->vqueue->desc[at_idx].len = sizeof(gpu_resource_create2d_request);
    gpu_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;

    gpu_device->vqueue->desc[(at_idx+1) % mod].addr = displayAddress;
    gpu_device->vqueue->desc[(at_idx+1) % mod].len = sizeof(gpu_display_info_response);
    gpu_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_WRITE;
    gpu_device->vqueue->desc[(at_idx+1) % mod].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    gpu_device->vqueue->queue_driver->ring[gpu_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    gpu_device->vqueue->queue_driver->idx += 1;

    //Update our index
    gpu_device->index = (gpu_device->index + 2) % mod;
    gpu_device->waiting_on_interrupt = 1;
    //gpu_device->waiting_on_config_interrupt = 1;


    //Notify the device that it has work to do
    gpu_notify();
    //printf("Currently gpu_device->waiting_on_interrupt: %d and gpu_device->waiting_on_config_interrupt %d\n",gpu_device->waiting_on_interrupt,gpu_device->waiting_on_config_interrupt);
    while(gpu_device->waiting_on_interrupt || gpu_device->waiting_on_config_interrupt){
        if(gpu_device->waiting_on_interrupt){
            //printf("still waiting on interrupt\n");
        }
    } 
}

//Attach frame buffer to gpu
void gpu_attach_resource(){
    //printf("gpu_attach_resource\n");
    uint64_t commandAddress;
    uint64_t memmoryAddress;
    uint64_t displayAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = gpu_device->index;
    mod = gpu_device->vinfo->type1->queue_size;


    gpu_resource_attach_backing * attach = pzalloc(sizeof(gpu_resource_attach_backing));
    gpu_mem_entry * memmory = pzalloc(sizeof(gpu_mem_entry));
    gpu_display_info_response * display_info = pzalloc(sizeof(gpu_display_info_response));
    attach->hdr.control_type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    attach->nr_entries = 1;
    attach->resource_id = 1;
    
    memmory->addr = mmu_translate(kernel_mmu_table, (uint64_t)gpu_device->frameBuffer);
    memmory->length = sizeof(Pixel) * gpu_device->width * gpu_device->height;
    memmory->padding = 0;

    //Virtio uses physical memory.
    commandAddress = mmu_translate(kernel_mmu_table, (uint64_t)attach);
    memmoryAddress = mmu_translate(kernel_mmu_table, (uint64_t)memmory);
    displayAddress = mmu_translate(kernel_mmu_table, (uint64_t)display_info);
    
    //Fill out descriptor
    gpu_device->vqueue->desc[at_idx].addr = commandAddress;
    gpu_device->vqueue->desc[at_idx].len = sizeof(gpu_resource_attach_backing);
    gpu_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;

    gpu_device->vqueue->desc[(at_idx+1) % mod].addr = memmoryAddress;
    gpu_device->vqueue->desc[(at_idx+1) % mod].len = sizeof(gpu_mem_entry);
    gpu_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[(at_idx+1) % mod].next = (at_idx+2);
    
    gpu_device->vqueue->desc[(at_idx+2) % mod].addr = displayAddress;
    gpu_device->vqueue->desc[(at_idx+2) % mod].len = sizeof(gpu_display_info_response);
    gpu_device->vqueue->desc[(at_idx+2) % mod].flags = VIRTQ_DESC_F_WRITE;
    gpu_device->vqueue->desc[(at_idx+2) % mod].next = 0;

    //Add descriptor's index to the driver ring at the driver queue index
    gpu_device->vqueue->queue_driver->ring[gpu_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    gpu_device->vqueue->queue_driver->idx += 1;

    //Update our index
    gpu_device->index = (gpu_device->index + 2) % mod;
    gpu_device->waiting_on_interrupt = 1;
    //gpu_device->waiting_on_config_interrupt = 1;


    //Notify the device that it has work to do
    gpu_notify();
    //printf("Currently gpu_device->waiting_on_interrupt: %d and gpu_device->waiting_on_config_interrupt %d\n",gpu_device->waiting_on_interrupt,gpu_device->waiting_on_config_interrupt);
    while(gpu_device->waiting_on_interrupt || gpu_device->waiting_on_config_interrupt){
        if(gpu_device->waiting_on_interrupt){
            //printf("still waiting on interrupt\n");
        }
    } 
}

//Choose what screen
void gpu_set_scanout(){
    uint64_t headerAddress;
    uint64_t displayAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = gpu_device->index;
    mod = gpu_device->vinfo->type1->queue_size;


    gpu_set_scanout_request * header = pzalloc(sizeof(gpu_set_scanout_request));
    gpu_display_info_response * display_info = pzalloc(sizeof(gpu_display_info_response));
    header->hdr.control_type = VIRTIO_GPU_CMD_SET_SCANOUT;
    header->rect.width = gpu_device->width;
    header->rect.height = gpu_device->height;
    header->resource_id = 1;
    header->scanout_id = 0;

    //VirtiO uses physical memory.
    headerAddress = mmu_translate(kernel_mmu_table, (uint64_t)header);
    displayAddress = mmu_translate(kernel_mmu_table, (uint64_t)display_info);
    
    //Fill out descriptor
    
    gpu_device->vqueue->desc[at_idx].addr = headerAddress;
    gpu_device->vqueue->desc[at_idx].len = sizeof(gpu_set_scanout_request);
    gpu_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;


    
    gpu_device->vqueue->desc[(at_idx+1) % mod].addr = displayAddress;
    gpu_device->vqueue->desc[(at_idx+1) % mod].len = sizeof(gpu_display_info_response);
    gpu_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_WRITE;
    gpu_device->vqueue->desc[(at_idx+1) % mod].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    gpu_device->vqueue->queue_driver->ring[gpu_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    gpu_device->vqueue->queue_driver->idx += 1;

    //Update our index
    gpu_device->index = (gpu_device->index + 2) % mod;
    gpu_device->waiting_on_interrupt = 1;
    //gpu_device->waiting_on_config_interrupt = 1;


    //Notify the device that it has work to do
    gpu_notify();
    //printf("Currently gpu_device->waiting_on_interrupt: %d and gpu_device->waiting_on_config_interrupt %d\n",gpu_device->waiting_on_interrupt,gpu_device->waiting_on_config_interrupt);
    while(gpu_device->waiting_on_interrupt || gpu_device->waiting_on_config_interrupt){
        if(gpu_device->waiting_on_interrupt){
            //printf("still waiting on interrupt\n");
        }
    }
}

//Transfer part of the frame buffer to the gpu 
void gpu_transfer_buffer(Rectangle *rect){
    //printf("gpu_transfer_buffer\n");
    uint64_t headerAddress;
    uint64_t displayAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = gpu_device->index;
    mod = gpu_device->vinfo->type1->queue_size;

    
    gpu_transfer_to_host_2d * header = pzalloc(sizeof(gpu_transfer_to_host_2d));
    gpu_display_info_response * display_info = pzalloc(sizeof(gpu_display_info_response));
    
    header->hdr.control_type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    header->rect.x = rect->x;
    header->rect.y = rect->y;
    header->rect.width = rect->width;
    header->rect.height = rect->height;
    header->resource_id = 1;
    //VirtiO uses physical memory.
    headerAddress = mmu_translate(kernel_mmu_table, (uint64_t)header);
    displayAddress = mmu_translate(kernel_mmu_table, (uint64_t)display_info);
    
    //Fill out descriptor
    gpu_device->vqueue->desc[at_idx].addr = headerAddress;
    gpu_device->vqueue->desc[at_idx].len = sizeof(gpu_transfer_to_host_2d);
    gpu_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;


    
    gpu_device->vqueue->desc[(at_idx+1) % mod].addr = displayAddress;
    gpu_device->vqueue->desc[(at_idx+1) % mod].len = sizeof(gpu_display_info_response);
    gpu_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_WRITE;
    gpu_device->vqueue->desc[(at_idx+1) % mod].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    gpu_device->vqueue->queue_driver->ring[gpu_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    gpu_device->vqueue->queue_driver->idx += 1;

    //Update our index
    gpu_device->index = (gpu_device->index + 2) % mod;
    gpu_device->waiting_on_interrupt = 1;
    //gpu_device->waiting_on_config_interrupt = 1;


    //Notify the device that it has work to do
    gpu_notify();
}

//Tell gpu to update the screen
void gpu_flush_buffer(){
    //printf("gpu_flush_buffer\n");
    uint64_t headerAddress;
    uint64_t displayAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = gpu_device->index;
    mod = gpu_device->vinfo->type1->queue_size;


    gpu_resource_flush * header = pzalloc(sizeof(gpu_resource_flush));
    gpu_display_info_response * display_info = pzalloc(sizeof(gpu_display_info_response));
    header->hdr.control_type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    header->rect.width = gpu_device->width;
    header->rect.height = gpu_device->height;
    header->resource_id = 1;
    

    //VirtiO uses physical memory.
    headerAddress = mmu_translate(kernel_mmu_table, (uint64_t)header);
    displayAddress = mmu_translate(kernel_mmu_table, (uint64_t)display_info);
    
    //Fill out descriptor
    
    gpu_device->vqueue->desc[at_idx].addr = headerAddress;
    gpu_device->vqueue->desc[at_idx].len = sizeof(gpu_resource_flush);
    gpu_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    gpu_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;


    
    gpu_device->vqueue->desc[(at_idx+1) % mod].addr = displayAddress;
    gpu_device->vqueue->desc[(at_idx+1) % mod].len = sizeof(gpu_display_info_response);
    gpu_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_WRITE;
    gpu_device->vqueue->desc[(at_idx+1) % mod].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    gpu_device->vqueue->queue_driver->ring[gpu_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    gpu_device->vqueue->queue_driver->idx += 1;

    //Update our index
    gpu_device->index = (gpu_device->index + 2) % mod;
    gpu_device->waiting_on_interrupt = 1;
    //gpu_device->waiting_on_config_interrupt = 1;


    //Notify the device that it has work to do
    gpu_notify();
}


//Fills a rectangle of the frame buffer with a color
void fill_rect(uint32_t screen_width, uint32_t screen_height, Pixel *buffer, Rectangle *rect, Pixel *fill_color){
   uint32_t top = rect->y;
   uint32_t bottom = rect->y + rect->height;
   uint32_t left = rect->x;
   uint32_t right = rect->x + rect->width;
   uint32_t row;
   uint32_t col;
   if (bottom > screen_height) bottom = screen_height;
   if (right > screen_width) right = screen_width;
   for (row = top;row < bottom;row++) {
      for (col = left;col < right;col++) {
          uint32_t offset = row * screen_width + col;
          buffer[offset] = *fill_color;
      }
   }
}

void gpu_update_frame(){
    Rectangle fullscreen = {0, 0, gpu_device->width, gpu_device->height};
    gpu_transfer_buffer(&fullscreen);
    gpu_flush_buffer();
}

Pixel *get_frame_buffer(int *width, int *height){
    *width = gpu_device->width;
    *height = gpu_device->height;
    return gpu_device->frameBuffer;
}
