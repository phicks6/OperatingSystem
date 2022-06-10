CROSS_COMPILE?=riscv64-unknown-linux-gnu-
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy
CFLAGS=-Wall -Wextra -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -Ilibc/ -mcmodel=medlow
LDFLAGS=-Tlds/libc.lds -Llibc
LIBS=-lc
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.elf,$(SOURCES))

all: $(OBJECTS)

%.elf: %.c Makefile libc/libc.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

libc/libc.a:
	$(MAKE) -C libc


.PHONY: clean
	
clean:
	$(MAKE) -C libc clean
	rm -f *.elf

