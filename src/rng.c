#include <rng.h>
#include <pcie.h>
#include <virtio.h>
#include <mmu.h>


RNGDevice * rng_device;


bool rng_probe(uint8_t bus, uint8_t slot, uint8_t function){
    rng_device->driver->bus = bus;
    rng_device->driver->slot = slot;
    return true;
}

RNGDevice * rng_create(){
    rng_device = pzalloc(sizeof(RNGDevice));
    rng_device->driver = pzalloc(sizeof(PcieDriver));
    rng_device->vqueue = pzalloc(sizeof(Virtq));
    rng_device->vinfo = pzalloc(sizeof(VirtioCapInfo));

    rng_device->driver->vendor_id = VIRTIO_VENDER;
    rng_device->driver->device_id = ENTROPY_DEVICE;
    rng_device->driver->probe = rng_probe;
    rng_device->driver->claim_irq = rng_claim_irq;
    rng_device->driver->init = rng_init;
    rng_device->index = 0;
    return rng_device;
}


bool rng_fill(void *buffer, uint16_t size){
    uint64_t phys;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = rng_device->index;
    mod = rng_device->vinfo->type1->queue_size;

    //Virtio uses physical memory.
    phys = mmu_translate(kernel_mmu_table, (uint64_t)buffer);

    //Fill out descriptor
    rng_device->vqueue->desc[at_idx].addr = phys;
    rng_device->vqueue->desc[at_idx].len = size;
    rng_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_WRITE;
    rng_device->vqueue->desc[at_idx].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    rng_device->vqueue->queue_driver->ring[rng_device->vqueue->queue_driver->idx % mod] = at_idx;

    //increment index on the driver letting it know it has work to do
    rng_device->vqueue->queue_driver->idx += 1;

    //Update our index
    rng_device->index = (rng_device->index + 1) % mod;
    rng_device->waiting_on_interrupt = 1;

    //Notify the device that it has work to do
    rng_notify();

    while(rng_device->waiting_on_interrupt){
        
    }
    return true;
}

//Claims a irq
bool rng_claim_irq(int irq){
    if(32 + ((rng_device->driver->bus + rng_device->driver->slot)%4) == irq){
        if(rng_device->vinfo->type3->queue_interrupt){
            rng_device->waiting_on_interrupt = 0;
            return true;
        }
        if(rng_device->vinfo->type3->device_cfg_interrupt){
            printf("Config interrupt\n");
            return true;
        }
    }
    return false;
}



void rng_init(){
    uint16_t preferedQsize = 16;
    
    //Reset device
    rng_device->vinfo->type1->device_status = 0;
    //Set acknowledge status bit
    rng_device->vinfo->type1->device_status = 1;
    //Set driver status bit
    rng_device->vinfo->type1->device_status |= 2;
    //Set features_ok status bit
    rng_device->vinfo->type1->device_status |= 8;

    //Select which queue we are working with
    rng_device->vinfo->type1->queue_select = 0;
    
    //Negotiate with device
    rng_device->vinfo->type1->queue_size = preferedQsize;
    uint16_t queueSize = rng_device->vinfo->type1->queue_size;
    //printf("negotiated Qsize = 0x%lx\n",rng_device->vinfo->type1->queue_size);


    //Set up memory for the descriptor table, driver ring and device ring 
    uint64_t virtAdd;
    virtAdd = (uint64_t)pzalloc(16 * queueSize); //Continuous
    rng_device->vqueue->desc = (struct virtq_desc *)virtAdd;
    rng_device->vinfo->type1->queue_desc = mmu_translate(kernel_mmu_table, virtAdd);

    virtAdd = (uint64_t)pzalloc(6 + 2 * queueSize); //Continuous
    rng_device->vqueue->queue_driver = (struct virtq_queue_driver *)virtAdd;
    rng_device->vinfo->type1->queue_driver = mmu_translate(kernel_mmu_table, virtAdd);
    
    virtAdd = (uint64_t)pzalloc(6 + 8 * queueSize); //Continuous
    rng_device->vqueue->queue_device = (struct virtq_queue_device *)virtAdd;
    rng_device->vinfo->type1->queue_device = mmu_translate(kernel_mmu_table, virtAdd);


    //Enable the queue 
    rng_device->vinfo->type1->queue_enable = 1;
    
    //Set device to live with the driver_ok bit which apparently gets cleared by writing the memory space bit of the ecam's command register
    rng_device->vinfo->type1->device_status |= 4;
}

//Gathers all the pieces to piece together the address to notify the device 
void rng_notify(void){
    uint64_t barAddress;
    uint64_t offset;
    uint64_t queue_notify_off;
    uint64_t notify_off_multiplier;

    volatile struct EcamHeader* ecam = pcie_get_ecam(rng_device->driver->bus, rng_device->driver->slot, rng_device->driver->function, 0);
    barAddress = pcie_read_bar(ecam,rng_device->vinfo->type2->cap.bar,64);
    offset = rng_device->vinfo->type2->cap.offset;
    queue_notify_off = rng_device->vinfo->type1->queue_notify_off; 
    notify_off_multiplier = rng_device->vinfo->type2->notify_off_multiplier;
    
    virtio_notify(barAddress,offset,queue_notify_off,notify_off_multiplier);
}