#include <uart.h>
#include <lock.h>
#include <printf.h>

#define UART_BUFFER_SIZE 32

struct Mutex uart_mutex;
char uart_buffer[UART_BUFFER_SIZE];
int uart_buffer_len = 0;
int uart_buffer_pos = 0;


void uart_init(void){
    volatile char *uart = (char*)0x10000000;
    uart[3] = 0b11; // Word length select bits set to 11 (3)
	uart[2] = 1; // FIFO enabled
	uart[1] = 1; // Interrupts enabled
}


char uart_get(void){
    volatile char *uart = (char*)0x10000000;
    if(uart[5] & 1){ //If data read bit
        return uart[0]; //Pull data from receiver buffer register
    }
    return 255;
    
}

void uart_put(char c){
    volatile char *uart = (char*)0x10000000;
    while(!(uart[5] & (1<<6))){ //If transmitter not ready wait
    }
    uart[0] = c; //Put data in transmitter holding register
}


char uart_buffer_get(void){ //Handles a read from the ring buffer
    char ret = 0xff;
    mutex_spinlock(&uart_mutex);
    if (uart_buffer_len > 0) { //as long as there is data in the buffer
        ret = uart_buffer[uart_buffer_pos]; //Get the value from the ring
        uart_buffer_pos = (uart_buffer_pos + 1) % UART_BUFFER_SIZE; //Bound the ring by buffer size
        uart_buffer_len -= 1;
    }else{
        mutex_unlock(&uart_mutex);
    }
    mutex_unlock(&uart_mutex);
    return ret;
}

void uart_handle_irq(void){
    char c;
    while ((c = uart_get()) != 0xff) { //While we get a value put it into the buffer
        mutex_spinlock(&uart_mutex);
        if (uart_buffer_len >= UART_BUFFER_SIZE) { 
            uart_buffer_pos += 1;
            uart_buffer_len -= 1;
        }
        uart_buffer[(uart_buffer_pos + uart_buffer_len) % UART_BUFFER_SIZE] = c;
        uart_buffer_len += 1;
        mutex_unlock(&uart_mutex);
    }
    
    
}