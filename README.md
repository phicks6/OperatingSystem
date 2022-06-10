# OperatingSystem
Contains the operating system that was programed for my Operating Systems: Design and Implementation class.

## About The Project
This OS was programmed from the ground up for operating systems class at The University of Tennessee Knoxville. This was a tough class do to the workload it entailed and this project is easily the biggest project that I've worked on by myself. It was designed for a RISC-V architechture with 8 harts in a Qemu emulation. It can be devided into two parts, the SBI and the actual OS. The SBI acts as a interface between machine mode and supervisor mode and acts as a bios that boots into the OS. 
Could you actually use this operating system? No, it is way to much to be actually useable and definitely has some bugs here and there. However, this was all coded in a single semester and I'm pretty proud of all that I was able to acomplish in that time.

### SBI Overview
The SBI is the only piece of code that runs in the machine mode, meaning that it is the only way to interact with the plic, clint, and machine mode registers. It exposes these services to the OS through the use of ecalls similar to how user programs request functionally from the OS with system calls.
The SBI sets up all the harts and puts harts 1-7 to sleep and has hart 0 load into the operating system. The code is located in the sbi directory and has a seperate makefile that creates an sbi.elf.

- clint.c:
        Allows us to use the mtimecmp register to interupt the program a set amount of time in the future.
- hart.c:
        Controls the waking, starting, and stopping of the harts.
- plic.c:
        Controls the Platform-Level Interrupt Controller allowing us to be able to handle interupts.
- uart.c:
        Controls uart keyboard capture and output to the terminal from the sbi.
- svcall.c:
        Deligates ecalls from supervisor mode to the proper submodule.
- trap_handler.S:
        Small piece of assembly that gets called whenever we trap into machine mode that imediatly passes functionally to trap.c.
- trap.c:
        Catches machine software interrupts, machine timer interrupts, machine external interrupts, and ecalls from supervisor mode.
- start.S:
        This is where Qemu starts executing instructions and what loads us into main.c.
 - main.c:
        This sets up all the harts with phyiscal memmory protection and sets up what types of interupts they can receive. It uses hart 0 as a bootstrap to load into the OS.
