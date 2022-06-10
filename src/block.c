#include <block.h>
#include <pcie.h>
#include <virtio.h>
#include <mmu.h>
#include <csr.h>
#include <util.h>

BlockDevice * block_device;


bool block_probe(uint8_t bus, uint8_t slot, uint8_t function){
    block_device->driver->bus = bus;
    block_device->driver->slot = slot;
    return true;
}

BlockDevice * block_create(){
    block_device = pzalloc(sizeof(BlockDevice));
    block_device->driver = pzalloc(sizeof(PcieDriver));
    block_device->vqueue = pzalloc(sizeof(Virtq));
    block_device->vinfo = pzalloc(sizeof(VirtioCapInfo));

    block_device->driver->vendor_id = VIRTIO_VENDER;
    block_device->driver->device_id = ENTROPY_DEVICE;
    block_device->driver->probe = block_probe;
    block_device->driver->claim_irq = block_claim_irq;
    block_device->driver->init = block_init;
    block_device->index = 0;
    
    return block_device;
}

//Preforms a read block request and copy only the requested data into the buffer
uint8_t block_read(void *buffer, uint64_t offset, uint64_t size){
    uint64_t sector = offset/block_device->blk_size;
    uint64_t num_sector = (((offset+size)/block_device->blk_size) - sector) + 1;
    
    void * temp_buffer = pzalloc(block_device->blk_size*num_sector);
    volatile uint8_t *status = pzalloc(sizeof(uint8_t));

    *status = 111;

    block_request(temp_buffer,status,sector,block_device->blk_size*num_sector,VIRTIO_BLK_T_IN);
    
    if(*status == 0){
        void * start = (temp_buffer + (offset - (sector*block_device->blk_size)));
        memcpy(buffer,start,size);
        pfree(temp_buffer);
        return 0;
    }

    printf("Error with status %d\n",*status);
    return *status;
    
}

//Preforms a write block request and handles gathering sectors of any partial sector writes
uint8_t block_write(void *buffer, uint64_t offset, uint64_t size){
    uint64_t sector = offset/block_device->blk_size;
    uint64_t num_sector = (((offset+size)/block_device->blk_size) - sector) + 1;
    void * temp_buffer = pzalloc(block_device->blk_size*num_sector);

    if(num_sector > 1){
        block_read(temp_buffer,sector,block_device->blk_size);
        block_read(temp_buffer+((num_sector-1)*block_device->blk_size),(sector+num_sector-1)*block_device->blk_size,block_device->blk_size);
    }else{
        block_read(temp_buffer,sector,block_device->blk_size);
    }

    void * start = (temp_buffer + (offset - (sector*block_device->blk_size)));
    memcpy(start,buffer, size);

    volatile uint8_t *status = pzalloc(sizeof(uint8_t));
    *status = 111;

    block_request(temp_buffer,status,sector,block_device->blk_size*num_sector,VIRTIO_BLK_T_OUT);
    
    if(*status == 0){
        void * start = (temp_buffer + (offset - (sector*block_device->blk_size)));
        pfree(temp_buffer);
        pfree(status);
        return 0;
    }

    printf("Error with status %d\n",*status);
    return *status;
}

//Bidirectional request to the block device
void block_request(void *buffer, uint8_t *status, uint16_t start_sector, uint16_t size, uint8_t direction){
    uint64_t headerAddress;
    uint64_t dataAddress;
    uint64_t statusAddress;
    uint32_t at_idx;
    uint32_t mod;

    at_idx = block_device->index;
    mod = block_device->vinfo->type1->queue_size;

    BlockHeader * header = pzalloc(sizeof(BlockHeader));
    header->type = direction;
    header->sector = start_sector;

    //VirtiO uses physical memory.
    headerAddress = mmu_translate(kernel_mmu_table, (uint64_t)header);
    dataAddress = mmu_translate(kernel_mmu_table, (uint64_t)buffer);
    statusAddress = mmu_translate(kernel_mmu_table, (uint64_t)status);


    //Fill out descriptor
    block_device->vqueue->desc[at_idx].addr = headerAddress;
    block_device->vqueue->desc[at_idx].len = sizeof(BlockHeader);
    block_device->vqueue->desc[at_idx].flags = VIRTQ_DESC_F_NEXT;
    block_device->vqueue->desc[at_idx].next = (at_idx+1) % mod;

    block_device->vqueue->desc[(at_idx+1) % mod].addr = dataAddress;
    block_device->vqueue->desc[(at_idx+1) % mod].len = size;
    
    if(direction){
        block_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_NEXT;
    }else{
        block_device->vqueue->desc[(at_idx+1) % mod].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
    }
    
    block_device->vqueue->desc[(at_idx+1) % mod].next = (at_idx+2) % mod;

    block_device->vqueue->desc[(at_idx+2) % mod].addr = statusAddress;
    block_device->vqueue->desc[(at_idx+2) % mod].len = sizeof(uint8_t);
    block_device->vqueue->desc[(at_idx+2) % mod].flags = VIRTQ_DESC_F_WRITE;
    block_device->vqueue->desc[(at_idx+2) % mod].next = 0;
    

    //Add descriptor's index to the driver ring at the driver queue index
    block_device->vqueue->queue_driver->ring[block_device->vqueue->queue_driver->idx % mod] = at_idx;
    
    //increment index on the driver letting it know it has work to do
    block_device->vqueue->queue_driver->idx += 1;
    uint32_t waiting_on_index = block_device->vqueue->queue_driver->idx % mod;
    //printf("waiting_on_index is %d\n",waiting_on_index);
    block_device->waiting_on_interrupt[waiting_on_index] = 1;
    //Update our index
    block_device->index = (block_device->index + 3) % mod;
    
    //Notify the device that it has work to do
    block_notify();

    //Ensures that we unblock after the data was populated of the request that we filled out in this call
    while(block_device->waiting_on_interrupt[waiting_on_index]){

    }
    return true;
}

//Claims a irq
bool block_claim_irq(int irq){
    if(32 + ((block_device->driver->bus + block_device->driver->slot)%4) == irq){
        if(block_device->vinfo->type3->queue_interrupt){
            //printf("Block claimed something\n");
            //block_device->waiting_on_interrupt = 0;

            
            //printf("block_device->vqueue->queue_device->idx %d\n",block_device->vqueue->queue_device->idx);
            block_device->waiting_on_interrupt[block_device->vqueue->queue_device->idx] = 0;
            return true;
        }
        if(block_device->vinfo->type3->device_cfg_interrupt){
            printf("Config interrupt\n");
            return true;
        }
    }
    return false;
}



void block_init(){
    uint16_t preferedQsize = 128;
    
    //Reset device
    block_device->vinfo->type1->device_status = 0;
    //Set acknowledge status bit
    block_device->vinfo->type1->device_status = 1;
    //Set driver status bit
    block_device->vinfo->type1->device_status |= 2;
    //Set features_ok status bit
    block_device->vinfo->type1->device_status |= 8;

    //Select which queue we are working with
    block_device->vinfo->type1->queue_select = 0;
    
    //Negotiate with device
    block_device->vinfo->type1->queue_size = preferedQsize;
    uint16_t queueSize = block_device->vinfo->type1->queue_size;
    block_device->qsize = queueSize;
    block_device->waiting_on_interrupt = pzalloc(sizeof(bool)*queueSize);

    //Set up memory for the descriptor table, driver ring and device ring 
    uint64_t virtAdd;
    virtAdd = (uint64_t)pzalloc(16 * queueSize); //Continuous
    block_device->vqueue->desc = (struct virtq_desc *)virtAdd;
    block_device->vinfo->type1->queue_desc = mmu_translate(kernel_mmu_table, virtAdd);

    virtAdd = (uint64_t)pzalloc(6 + 2 * queueSize); //Continuous
    block_device->vqueue->queue_driver = (struct virtq_queue_driver *)virtAdd;
    block_device->vinfo->type1->queue_driver = mmu_translate(kernel_mmu_table, virtAdd);
    
    virtAdd = (uint64_t)pzalloc(6 + 8 * queueSize); //Continuous
    block_device->vqueue->queue_device = (struct virtq_queue_device *)virtAdd;
    block_device->vinfo->type1->queue_device = mmu_translate(kernel_mmu_table, virtAdd);


    //Enable the queue 
    block_device->vinfo->type1->queue_enable = 1;
    
    //Set device to live with the driver_ok bit which apparently gets cleared by writing the memory space bit of the ecam's command register
    block_device->vinfo->type1->device_status |= 4;
}

//Gathers all the pieces to piece together the address to notify the device 
void block_notify(void){
    uint64_t barAddress;
    uint64_t offset;
    uint64_t queue_notify_off;
    uint64_t notify_off_multiplier;

    volatile struct EcamHeader* ecam = pcie_get_ecam(block_device->driver->bus, block_device->driver->slot, block_device->driver->function, 0);
    barAddress = pcie_read_bar(ecam,block_device->vinfo->type2->cap.bar,64);
    offset = block_device->vinfo->type2->cap.offset;
    queue_notify_off = block_device->vinfo->type1->queue_notify_off; 
    notify_off_multiplier = block_device->vinfo->type2->notify_off_multiplier;
    
    virtio_notify(barAddress,offset,queue_notify_off,notify_off_multiplier);
}