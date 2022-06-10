#include <plic.h>
#include <stdint.h>
#include <printf.h>
#include <uart.h>

void plic_set_priority(int interrupt_id, char priority)
{
    uint32_t *base = (uint32_t *)PLIC_PRIORITY(interrupt_id);
    *base = priority & 0x7;
}
void plic_set_threshold(int hart, char priority)
{
    uint32_t *base = (uint32_t *)PLIC_THRESHOLD(hart, PLIC_MODE_MACHINE);
    *base = priority & 0x7;
}
void plic_enable(int hart, int interrupt_id)
{
    uint32_t *base = (uint32_t *)PLIC_ENABLE(hart, PLIC_MODE_MACHINE);
    base[interrupt_id / 32] |= 1UL << (interrupt_id % 32);
}
void plic_disable(int hart, int interrupt_id)
{
    uint32_t *base = (uint32_t *)PLIC_ENABLE(hart, PLIC_MODE_MACHINE);
    base[interrupt_id / 32] &= ~(1UL << (interrupt_id % 32));
}
uint32_t plic_claim(int hart)
{
    uint32_t *base = (uint32_t *)PLIC_CLAIM(hart, PLIC_MODE_MACHINE);
    return *base;
}
void plic_complete(int hart, int id)
{
    uint32_t *base = (uint32_t *)PLIC_CLAIM(hart, PLIC_MODE_MACHINE);
    *base = id;
}


void plic_handle_irq(int hart){
    int irq = plic_claim(hart);
    switch(irq){ //What type of interrupt
        case 10: //UART
            uart_handle_irq();// in uart.c
            break;
        default:
            printf("Unhandled irq %d\n\n",irq);
            break;
    }
    plic_complete(hart,irq);
    
    
}