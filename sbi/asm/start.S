#This is where our sbi starts, and sends us into main of main.c
.section .text.init
.global _start
_start:
    csrr a0, mhartid
    la  sp, _stack_end
    la	gp, __global_pointer$
    li  t0, 4096
    mul t0, a0, t0
    sub sp, sp, t0

    tail main

#This is a infinite loop to keep a hart somewhere
.section .text
.global park
park:
    wfi
    j   park