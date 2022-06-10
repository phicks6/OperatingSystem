#include <virtio.h>
#include <pcie.h>
#include <mmu.h>
#include <rng.h>
#include <block.h>
#include <gpu.h>
#include <input.h>

extern RNGDevice * rng_device;
extern BlockDevice * block_device;
extern GPUDevice * gpu_device;
extern InputDevice * input_device;
extern uint8_t input_device_index;

//Create and register all the drivers we need
void virtio_init(){
    rng_create();
    pcie_register_driver(VIRTIO_VENDER,ENTROPY_DEVICE,rng_device->driver);
    block_create();
    pcie_register_driver(VIRTIO_VENDER,BLOCK_DEVICE,block_device->driver);
    gpu_create();
    pcie_register_driver(VIRTIO_VENDER,GPU_DEVICE,gpu_device->driver);
    input_create();
    pcie_register_driver(VIRTIO_VENDER,INPUT_DEVICE,input_device->driver);
}

//Calculates the address to write 0 into to notify the device
void virtio_notify(uint64_t barAddress,uint64_t offset ,uint64_t queue_notify_off,uint64_t notify_off_multiplier){
    uint64_t combinedAddress = barAddress+offset;
    uint64_t notifyAddress = BAR_NOTIFY_CAP(combinedAddress, queue_notify_off, notify_off_multiplier );
    *((uint32_t*)notifyAddress) = 0;
}


//Sets the address of the capabilities to their respective driver
void virtio_set_type1(uint16_t device_id, virtio_pci_common_cfg *type1){
    switch(device_id){
    case ENTROPY_DEVICE:
        rng_device->vinfo->type1 = type1;
        break;
    case BLOCK_DEVICE:
        block_device->vinfo->type1 = type1;
        break;
    case GPU_DEVICE:
        gpu_device->vinfo->type1 = type1;
        break;
    case INPUT_DEVICE:
        input_device[input_device_index].vinfo->type1 = type1;
        break;
    default:
        break;
    }
}
void virtio_set_type2(uint16_t device_id, virtio_pci_notify_cap *type2){
    switch(device_id){
    case ENTROPY_DEVICE:
        rng_device->vinfo->type2 = type2;
        break;
    case BLOCK_DEVICE:
        block_device->vinfo->type2 = type2;
        break;
    case GPU_DEVICE:
        gpu_device->vinfo->type2 = type2;
        break;
    case INPUT_DEVICE:
        input_device[input_device_index].vinfo->type2 = type2;
        break;
    default:
        break;
    }
}
void virtio_set_type3(uint16_t device_id, virtio_pci_isr_cap *type3){
    switch(device_id){
    case ENTROPY_DEVICE:
        rng_device->vinfo->type3 = type3;
        break;
    case BLOCK_DEVICE:
        block_device->vinfo->type3 = type3;
        break;
    case GPU_DEVICE:
        gpu_device->vinfo->type3 = type3;
        break;
    case INPUT_DEVICE:
        input_device[input_device_index].vinfo->type3 = type3;
        break;
    default:
        break;
    }
}
void virtio_set_type4(uint16_t device_id, void *type4){
    switch(device_id){
    case ENTROPY_DEVICE:
        //nothing needed
        break;
    case BLOCK_DEVICE:
        block_device->type4 = type4;
        block_device->blk_size = block_device->type4->blk_size;
        break;
    case GPU_DEVICE:
        gpu_device->type4 = type4;
        break;
    case INPUT_DEVICE:
        input_device[input_device_index].type4 = type4;
        break;
    default:
        break;
    }
}