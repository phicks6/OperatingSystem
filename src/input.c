#include <input.h>
#include <pcie.h>
#include <virtio.h>
#include <mmu.h>
#include <input-event-codes.h>

InputDevice * input_device;
uint8_t input_device_index = 0;
RingBuffer *keyboard_buffer;
RingBuffer *tablet_buffer;
RingBuffer *click_buffer;

int mouse_x = 0;
int mouse_y = 0;

bool left_clicking = false;
bool right_clicking = false;

bool input_probe(uint8_t bus, uint8_t slot, uint8_t function){
    input_device[input_device_index].driver->bus = bus;
    input_device[input_device_index].driver->slot = slot;
    //printf("Device %d has bus %d slot %d\n",input_device_index,bus,slot);
    return true;
}

InputDevice * input_create(){
    input_device = pzalloc(sizeof(InputDevice)*2);

    input_device[0].driver = pzalloc(sizeof(PcieDriver));
    input_device[0].vqueue = pzalloc(sizeof(Virtq));
    input_device[0].vinfo = pzalloc(sizeof(VirtioCapInfo));

    input_device[0].driver->vendor_id = VIRTIO_VENDER;
    input_device[0].driver->device_id = ENTROPY_DEVICE;
    input_device[0].driver->probe = input_probe;
    input_device[0].driver->claim_irq = input_claim_irq;
    input_device[0].driver->init = input_init;
    input_device[0].index = 0;

    input_device[1].driver = pzalloc(sizeof(PcieDriver));
    input_device[1].vqueue = pzalloc(sizeof(Virtq));
    input_device[1].vinfo = pzalloc(sizeof(VirtioCapInfo));

    input_device[1].driver->vendor_id = VIRTIO_VENDER;
    input_device[1].driver->device_id = ENTROPY_DEVICE;
    input_device[1].driver->probe = input_probe;
    input_device[1].driver->claim_irq = input_claim_irq;
    input_device[1].driver->init = input_init;
    input_device[1].index = 0;


    keyboard_buffer = pzalloc(sizeof(RingBuffer));
    tablet_buffer = pzalloc(sizeof(RingBuffer));
    click_buffer = pzalloc(sizeof(RingBuffer));

    keyboard_buffer->pos = 0;
    tablet_buffer->pos = 0;
    click_buffer->pos = 0;
    keyboard_buffer->num_elements = 0;
    tablet_buffer->num_elements = 0;
    click_buffer->num_elements = 0;
    keyboard_buffer->element_size = sizeof(input_event);
    tablet_buffer->element_size = sizeof(input_event);
    click_buffer->element_size = sizeof(input_event);
    keyboard_buffer->capacity = 256;
    tablet_buffer->capacity = 1024;
    click_buffer->capacity = 32;
    keyboard_buffer->behavior = RB_OVERWRITE;
    tablet_buffer->behavior = RB_OVERWRITE;
    click_buffer->behavior = RB_OVERWRITE;
    keyboard_buffer->buffer = pzalloc(sizeof(input_event)*keyboard_buffer->capacity);
    tablet_buffer->buffer = pzalloc(sizeof(input_event)*tablet_buffer->capacity);
    click_buffer->buffer = pzalloc(sizeof(input_event)*click_buffer->capacity);
    


    return input_device;
}


//Claims a irq
bool input_claim_irq(int irq){
    //printf("Tablet %d and keyboard %d\n",((input_device[0].driver->bus + input_device[0].driver->slot)%4), ((input_device[1].driver->bus + input_device[1].driver->slot)%4));
    if(32 + ((input_device[0].driver->bus + input_device[0].driver->slot)%4) == irq){
        uint32_t interuptThing =  *(uint32_t*)input_device[0].vinfo->type3;
        virtio_pci_isr_cap *temp = &interuptThing;

        if(temp->queue_interrupt){
            input_device[0].waiting_on_interrupt = 0;
            //printf("Now look in the device ring\n\n");
            input_copy_to_internal(0);
            

            return true;
        }
        if(temp->device_cfg_interrupt){
            printf("Config interrupt\n");
            return true;
        }
    }
    if(32 + ((input_device[1].driver->bus + input_device[1].driver->slot)%4) == irq){
        uint32_t interuptThing =  *(uint32_t*)input_device[1].vinfo->type3;
        virtio_pci_isr_cap *temp = &interuptThing;

        if(temp->queue_interrupt){
            input_device[1].waiting_on_interrupt = 0;
            //printf("Now look in the device ring\n\n");
            input_copy_to_internal(1);

            return true;
        }
        if(temp->device_cfg_interrupt){
            printf("Config interrupt\n");
            return true;
        }
    }
    return false;
}

void input_init(){
    uint16_t preferedQsize = 64;
    
    //Reset device
    input_device[input_device_index].vinfo->type1->device_status = 0;
    //Set acknowledge status bit
    input_device[input_device_index].vinfo->type1->device_status = 1;
    //Set driver status bit
    input_device[input_device_index].vinfo->type1->device_status |= 2;
    //Set features_ok status bit
    input_device[input_device_index].vinfo->type1->device_status |= 8;

    //Select which queue we are working with
    input_device[input_device_index].vinfo->type1->queue_select = 0;
    
    //Negotiate with device
    input_device[input_device_index].vinfo->type1->queue_size = preferedQsize;
    uint16_t queueSize = input_device[input_device_index].vinfo->type1->queue_size;
    input_device[input_device_index].qsize = queueSize;


    //Set up memory for the descriptor table, driver ring and device ring 
    uint64_t virtAdd;
    virtAdd = (uint64_t)pzalloc(16 * queueSize); //Continuous
    input_device[input_device_index].vqueue->desc = (struct virtq_desc *)virtAdd;
    input_device[input_device_index].vinfo->type1->queue_desc = mmu_translate(kernel_mmu_table, virtAdd);


    


    virtAdd = (uint64_t)pzalloc(6 + 2 * queueSize); //Continuous
    input_device[input_device_index].vqueue->queue_driver = (struct virtq_queue_driver *)virtAdd;
    input_device[input_device_index].vinfo->type1->queue_driver = mmu_translate(kernel_mmu_table, virtAdd);
    
    input_device[input_device_index].vqueue->queue_driver->idx += queueSize;


    virtAdd = (uint64_t)pzalloc(6 + 8 * queueSize); //Continuous
    input_device[input_device_index].vqueue->queue_device = (struct virtq_queue_device *)virtAdd;
    input_device[input_device_index].vinfo->type1->queue_device = mmu_translate(kernel_mmu_table, virtAdd);

    

    //Enable the queue 
    input_device[input_device_index].vinfo->type1->queue_enable = 1;
    
    //Set device to live with the driver_ok bit which apparently gets cleared by writing the memory space bit of the ecam's command register
    input_device[input_device_index].vinfo->type1->device_status |= 4;

    virtAdd = (uint64_t)pzalloc(sizeof(input_event) * queueSize);
    input_device[input_device_index].event_buffer = (input_event *)virtAdd;
    for(int i = 0; i < queueSize; i++){
        input_device[input_device_index].vqueue->desc[i].addr = mmu_translate(kernel_mmu_table, &(input_device[input_device_index].event_buffer[i]));
        input_device[input_device_index].vqueue->desc[i].len = sizeof(input_event);
        input_device[input_device_index].vqueue->desc[i].flags = VIRTQ_DESC_F_WRITE;
        input_device[input_device_index].vqueue->desc[i].next = 0;
        input_device[input_device_index].vqueue->queue_driver->ring[i] = i;
    }
    input_device[input_device_index].vqueue->queue_driver->idx += queueSize;

    input_device[input_device_index].type4->select = VIRTIO_INPUT_CFG_ID_NAME;
    input_device[input_device_index].type4->subsel = 0;
    //printf("%.128s", input_device[input_device_index].type4->string);
    input_device[input_device_index].type4->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
    input_device[input_device_index].type4->subsel = 0;
    if(input_device[input_device_index].type4->ids.product == EV_KEY){
        //printf("Found keyboard input device.\n");
    }
    else if(input_device[input_device_index].type4->ids.product = EV_ABS) {
        //printf("Found tablet input device.\n");
    }else{
        //printf("Found an input device product id %d\n", input_device[input_device_index].type4->ids.product);
    }

    input_device_index++;
}


//Upon being interupted, copy all new data from the device queue into a ring buffer
void input_copy_to_internal(int interupt_index){
    int numEvents = -1;
    if((input_device[interupt_index].vqueue->queue_device->idx % input_device[interupt_index].qsize) < input_device[interupt_index].index){
        numEvents = input_device[interupt_index].qsize-input_device[interupt_index].index + (input_device[interupt_index].vqueue->queue_device->idx % input_device[interupt_index].qsize);
    }else if((input_device[interupt_index].vqueue->queue_device->idx % input_device[interupt_index].qsize) == input_device[interupt_index].index){
        //numEvents = input_device[interupt_index].qsize;
    }else{
        numEvents = (input_device[interupt_index].vqueue->queue_device->idx % input_device[interupt_index].qsize) - input_device[interupt_index].index;
    }

    if(numEvents == -1){
        return;
    }

    for(int i = 0; i < numEvents; i++){
        int index = (input_device[interupt_index].index+i) % input_device[interupt_index].qsize;
        //Did the keyboard or mouse interupt us
        if(interupt_index == 0){
            if(input_device[interupt_index].event_buffer[index].type == EV_ABS){
                ring_push(tablet_buffer,&(input_device[interupt_index].event_buffer[index]));
            }else if(input_device[interupt_index].event_buffer[index].type == EV_KEY){
                ring_push(tablet_buffer,&(input_device[interupt_index].event_buffer[index]));
            }else{
            }
        }else{
            if(input_device[interupt_index].event_buffer[index].type == EV_KEY){
                ring_push(keyboard_buffer,&(input_device[interupt_index].event_buffer[index]));
            }else{
            }
        }
    }

    input_device[interupt_index].index = (input_device[interupt_index].index + numEvents) % input_device[interupt_index].qsize;
    input_device[interupt_index].vqueue->queue_driver->idx += numEvents;
}


bool ring_push(RingBuffer *buff,  void *data){
    if(buff->num_elements >= buff->capacity){
        switch(buff->behavior){
            case RB_DISCARD:
                printf("Buff full\n");
                return true;
            break;
        }

        
    }
    uint32_t first_empty_index = (buff->pos + buff->num_elements) % buff->capacity;
    uint64_t myBuffAdd = buff->buffer+(first_empty_index*buff->element_size);
    memcpy(myBuffAdd,data,buff->element_size);
    buff->num_elements++;
    return true;
}

bool ring_pop(RingBuffer *buff, void *data){

    
    if(buff->num_elements == 0){
        return false;
    }
    //printf("Buff has %d elements\n",buff->num_elements);
    uint64_t myBuffAdd = (buff->buffer+(buff->pos*buff->element_size));
    memcpy(data,myBuffAdd,buff->element_size);
    buff->pos = (buff->pos + 1) % buff->capacity;
    buff->num_elements--;
    return true;
}


//Loop to display any mouse or keyboard events
void vomit_inputs(){
    while(1){
        input_event event;
        while(ring_pop(keyboard_buffer,&event)){
            printf("Keyboard event:\n\tcode: %d\n\tvalue: %d\n",event.code,event.value);
        }
        while(ring_pop(click_buffer,&event)){
            printf("Click event:\n\tcode: %d\n\tvalue: %d\n",event.code,event.value);
        }
        while(ring_pop(tablet_buffer,&event)){
            printf("Tablet event:\n\tcode: %d\n\tvalue: %x\n",event.code,event.value);
        }
    }
}

bool get_tablet_buffer_event(input_event *event){
    return ring_pop(tablet_buffer,event);
}

//Keeps position of mouse tracked so it can be given to userspace appllication using system calls. Also keeps track of the being clicked down or not.
void track_mouse(){
    int screen_width = 1200;
    int screen_height = 800;
    get_frame_buffer(&screen_width,&screen_height);
    
    while(1){
        input_event event;
        while(get_tablet_buffer_event(&event)){
            //printf("Tablet event:\n\tcode: %d\n\tvalue: %x\n",event.code,event.value); 
            if(event.type == EV_KEY){
                printf("Click %d %d\n",event.code,event.value);
                if(event.code == 272){
                    if(event.value == 1){
                        left_clicking = true;
                            
                    }else{
                        left_clicking = false;
                    }
                }else if(event.code == 273){
                    if(event.value == 1){
                        right_clicking = true;
                    }else{
                        right_clicking = false;
                    }
                }
            }else if(event.type == EV_ABS){
                float prec = (event.value)/32767.0;
                if(event.code == 0){
                    //printf("First is x\n");
                    mouse_x = prec*screen_width;
                }else if(event.code == 1){
                    //printf("First is y\n");
                    mouse_y = prec*screen_height;
                }

                if(!get_tablet_buffer_event(&event)){
                    printf("Not an x and y pair!\n");
                }
                prec = (event.value)/32767.0;
                if(event.code == 0){
                    //printf("second is x\n");
                    mouse_x = prec*screen_width;
                }else if(event.code == 1){
                    //printf("second is y\n");
                    mouse_y = prec*screen_height;
                }
            }            
        }
    }
}

int get_mouse_x(void){
    return mouse_x;
}
int get_mouse_y(void){
    return mouse_y;
}

int get_mouse_state(void){
    if(left_clicking){
        if(right_clicking){
            return 3;
        }else{
            return 1;
        }
    }else if(right_clicking){
        return 2;
    }else{
        return 0;
    }
    
}
