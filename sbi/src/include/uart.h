#pragma once

void uart_init(void);

char uart_get(void);

char uart_buffer_get(void);

void uart_put(char c);

void uart_handle_irq(void);