#include <stdint.h>
#include <pcie.h>
#include <printf.h>
#include <mmu.h>
#include <malloc.h>
#include <util.h>
#include <virtio.h>
#include <rng.h>

#define MMIO_ECAM_BASE 0x30000000
#define BAR_BASE 0x40000000
DriverList* driver_list_head;


//Reads through the capabilities of a device and passes the infomation to the virtio subsystem.
static void pcie_enumerate_capabilities(volatile struct EcamHeader *ecam){
    if( ((ecam->status_reg >> 4) & 1) != 0 ){ //If capabilities list is 0 it can't do anything
        struct Capability *cap;
        unsigned char next_cape_offset = ecam->common.capes_pointer;
       
        while(next_cape_offset != 0){
            cap = (struct Capability *) (((unsigned long)ecam) + next_cape_offset);
            //printf("\tFound cape at offset 0x%lx location 0x%lx\n",next_cape_offset, cap);
            switch(cap->id){
                case VENDER_SPECIFIC:{
                    struct virtio_pci_cap *capv;
                    capv = (struct virtio_pci_cap *)cap;
                    uint64_t bar_address = pcie_read_bar(ecam,capv->bar,32);
                    switch(capv->cfg_type){
                        case VIRTIO_PCI_CAP_COMMON_CFG:{
                            uint64_t struct_address = bar_address + capv->offset;
                            struct virtio_pci_common_cfg * type1 = struct_address;
                            virtio_set_type1(ecam->device_id,type1);
                        }
                        break;
                        case VIRTIO_PCI_CAP_NOTIFY_CFG:{
                            uint64_t struct_address = capv;
                            struct virtio_pci_notify_cap * type2 = struct_address;
                            virtio_set_type2(ecam->device_id,type2);
                        }
                        break;
                        case VIRTIO_PCI_CAP_ISR_CFG:{
                            uint64_t struct_address = bar_address + capv->offset;
                            struct virtio_pci_isr_cap * type3 = struct_address;
                            virtio_set_type3(ecam->device_id,type3);

                        }
                        break;
                        case VIRTIO_PCI_CAP_DEVICE_CFG:{
                            uint64_t struct_address = bar_address + capv->offset;
                            void * type4 = struct_address;
                            virtio_set_type4(ecam->device_id,type4);

                        }
                        break;
                        default:

                        break;
                    }

                }
                break;
                case PCIE:
                break;
                default:
                    //printf("\nUnhandled Capability 0x%02x\n",cap->id);
                break;
            }

            next_cape_offset = cap->next;
        }
    }
}

//Enumerates the bus finding and bridges or devices and sets them up
void enumerateBus(void){
    printf("Attached PCIE Devices\n");
    for(int bus = 0; bus < 256; bus++){
        for(int slot = 0; slot < 32; slot++){
            volatile struct EcamHeader* ecam = pcie_get_ecam(bus,slot,0,0);
            if(ecam->vendor_id != 0xffff){
                printf("\tDevice at bus %d, slot %d (MMIO @ 0x%08lx), class: 0x%04x, header_type: (%d)\n",bus, slot, ecam, ecam->class_code, ecam->header_type);
                printf("\t\tDevice ID    : 0x%04x, Vendor ID    : 0x%04x\n", ecam->device_id, ecam->vendor_id);
                if(ecam->header_type == 1){//Bridge things
                    pcie_setup_bridge(ecam,bus);
                }else if(ecam->header_type == 0){
                    pcie_setup_device(ecam,bus,slot);
                }
                
            }
        }
    }
    printf("\n");
}

//Reads the address in a bar
uint64_t pcie_read_bar(volatile struct EcamHeader *ecam, uint8_t bar, uint8_t bar_size){
    if(bar_size == 64){
        uint32_t result = ecam->type0.bar[bar];
        result = result & 0xFFFF0000;

        uint32_t lower_result = ecam->type0.bar[bar+1];
        lower_result = lower_result & 0xFFFF0000;
        return (result << 32) | lower_result;
    }else{
        uint32_t result = ecam->type0.bar[bar];
        result = result & 0xFFFF0000;
        return result;
    }
    
}

//Sets up a device by mapping any used bars
static void pcie_setup_device(volatile struct EcamHeader *ecam, uint8_t bus, uint8_t slot){
    static uint8_t offset = 0;
    
    if(bus == 0 && slot == 0){//Ignore root port 0
        return;
    }

    for(int i = 0; i < 6; i++){ //Loop through BARs
        uint32_t result = ecam->type0.bar[i];
        ecam->type0.bar[i] = (uint32_t) -1;
        result = ecam->type0.bar[i];
        if(result != 0){
            if(result&(1<<2)){ //BAR 64 bits
                uint32_t address = BAR_BASE + (0x1000000 * offset) + (0x100000 * i);
                mmu_map_multiple(kernel_mmu_table, address, address, 0x100000, PB_READ | PB_WRITE);//BARs
                ecam->type0.bar[i] = address;
                i++;
            }else{
                uint32_t address = BAR_BASE + (0x1000000 * offset) + (0x100000 * i);
                mmu_map_multiple(kernel_mmu_table, address, address, 0x100000, PB_READ | PB_WRITE);//BARs
                ecam->type0.bar[i] = address;
                result = ecam->type0.bar[i];
            }
            
        }
        
    }
    ecam->command_reg = ecam->command_reg | (0b10);
    offset++;
    PcieDriver * d = pcie_find_driver(ecam->vendor_id,ecam->device_id);
    pcie_enumerate_capabilities(ecam);
    
    uint8_t function = 0;
    if(d!=NULL && d->probe(bus,slot,function)){
        if(d && d->init){
            d->function = 0;
            d->init();
        }
    }
}


//Make bridge forward everything in our possible bar range
static void pcie_setup_bridge(volatile struct EcamHeader *ecam, uint8_t bus){
    static uint8_t pci_bus_no = 1;
    ecam->command_reg = (1<<2) | (1<<1); // Bus master and memory space
    ecam->type1.memory_base = 0x4000;
    ecam->type1.memory_limit = 0x4fff;
    ecam->type1.prefetch_memory_limit = 0x4fff;
    ecam->type1.prefetch_memory_base = 0x4000;
    ecam->type1.primary_bus_no = bus;
    ecam->type1.secondary_bus_no = pci_bus_no;
    ecam->type1.subordinate_bus_no = pci_bus_no;
    pci_bus_no += 1;
}


//Puts a driver into our driverlist
void pcie_register_driver(uint16_t vendor_id, uint16_t device_id, PcieDriver *driver){
    DriverList *d = (DriverList*)pzalloc(sizeof(DriverList));
    d->driver = driver;
    d->vendor_id = vendor_id;
    d->device_id = device_id;
    d->next = driver_list_head;
    driver_list_head = d;
}

//Finds a driver that matches the vendor id and device id
PcieDriver * pcie_find_driver(uint16_t vendor_id, uint16_t device_id){
    DriverList *d = driver_list_head;
    while(d != NULL){
        if(d->vendor_id == vendor_id && d->device_id == device_id){
            return d->driver;
        }
        d = d->next;
    }
    return NULL;
}

//Has all the drivers ask their device if they were the cause of the interupt
bool pcie_claim_irq(int irq){
    DriverList *d;
    for(d = driver_list_head; d !=NULL; d = d->next){
        if(d->driver->claim_irq){ //Driver has a function set up
            if(d->driver->claim_irq(irq)){ //Claim irq returned true
                return true;
            }
        }
    }
    return false;
}


void init_pcie(int uh){
    driver_list_head = NULL;
    virtio_init();
    enumerateBus();
}