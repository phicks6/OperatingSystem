#pragma once

void console(int hart);
void run_cmd(char command[]);
void test_hart(void);
static void start_hart(unsigned int hart);
static void memtest(void);