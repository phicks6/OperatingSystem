# OperatingSystem
Contains the operating system that was programed for my Operating Systems: Design and Implementation class.

## About The Project
This OS was programmed from the ground up for operating systems class at The University of Tennessee Knoxville. This was a tough class due to the workload it entailed and this project is easily the biggest project that I've worked on by myself. It was designed for a RISC-V architecture with 8 harts in a Qemu emulation. It can be divided into two parts, the SBI, and the actual OS. The SBI acts as an interface between machine mode and supervisor mode and acts as a bios that boots into the OS.  
Could you actually use this operating system? No, it lacks way too much to be actually useable and definitely has some bugs here and there. However, this was all coded in a single semester and I'm pretty proud of all that I was able to accomplish in that time.

### SBI Overview
The SBI is the only piece of code that runs in the machine mode, meaning that it is the only way to interact with the plic, clint, and machine mode registers. It exposes these services to the OS through the use of ecalls similar to how user programs request functionally from the OS with system calls.
The SBI sets up all the harts and puts harts 1-7 to sleep and has hart 0 load into the operating system. The code is located in the sbi directory and has a separate makefile that creates an sbi.elf.  

- clint.c:
        Allows us to use the mtimecmp register to interrupt the program a set amount of time in the future.
- hart.c:
        Controls the waking, starting, and stopping of the harts.
- plic.c:
        Controls the Platform-Level Interrupt Controller allowing us to be able to handle interrupts.
- uart.c:
        Controls uart keyboard capture and output to the terminal from the sbi.
- svcall.c:
        Delegates ecalls from supervisor mode to the proper submodule.
- trap_handler.S:
        Small piece of assembly that gets called whenever we trap into machine mode that immediately passes functionally to trap.c.
- trap.c:
        Catches machine software interrupts, machine timer interrupts, machine external interrupts, and ecalls from supervisor mode.
- start.S:
        This is where Qemu starts executing instructions and what loads us into main.c.
 - main.c:
        This sets up all the harts with physical memory protection and sets up what types of interrupts they can receive. It uses hart 0 as a bootstrap to load into the OS.

### OS Overview:
The OS runs in supervisor mode which uses virtual addresses so translating the addresses was necessary. It had a page allocator that is primarily used to feed my own version of malloc. The OS is designed to support 5 VirtIO devices through virtual PCIe connections: an entropy device, block device(hard drive), gpu, keyboard, and tablet(mouse). It uses a CFS to schedule processes on the 8 harts. It can read minix3 filesystems and can execute compiled elf files in user mode. 

- sbi.c:
        Wrappers for assembly functions to execute ecalls to the sbi.
- mmu.c:
        Adds support for virtual addressing. Only the sbi will be using only physical addresses.
- malloc.c:
        Contains similar functions to standard malloc implementations but ensures physical continuity in the memory as that is necessary for other operations in the OS. It gets its address from the page allocator in page.c.
- pcie.c:
        Enables support for PCIe devices. Enumerates the bus looking for connected devices, sets up the BARs (base address registers), enumerates their capabilities, and attempts to match them with a registered driver.
- virtio.c:
        Handles the Virtio specifications and functionally like notifying devices.
- rng.c:
        Driver for the virtual RNG device.
- block.c:
        Driver for reading and writing blocks of a disk.
- input.c:
        Driver for keyboard and mouse input. Keeps separate ring buffers from devices so inputs aren't dropped as frequently.
- gpu.c:
        GPU driver that allows you to display stuff to the screen.
- process.c:
        Allows for the creation of new processes and allows them to be spawned on a hart.
- schedular.c:
        Enables a Completely Fair Scheduler (CFS) that chooses the process with the least amount runtime to run on an available hart.
- minix3.c:
        Enables reading of disks that use the minix3 filesystem.
- fs.c:
        Creates a virtual filesystem that allows for combining multiple disks into one seamless filesystem.
- elf.c:
        Reads elf files and loads them into a process so they can be scheduled and run.
- trap.c:
        Handles timer interrupts, system calls from user mode and identifies various types of faults.
- main.c:
        Maps memory, sets up interrupts, and inits schedular, pcie subsystems, and the filesystem. Schedules the user paint.elf process.
