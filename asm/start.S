#Hart 0 makes it here after MRETing in sbi/main.c
.section .text.init
.global _start
_start:
.option push
.option norelax
    la    sp, _stack_end
    la    gp, __global_pointer$
.option pop
    la t0, main
    csrw    sepc, t0
    la  t0, _asm_trap_vector #Set a trap handler
    csrw    stvec, t0
    li  t0, 1<<9
    csrw    sie, t0
    li  t0, (1<<13) | (1<<8) | (1<<5)
    csrw sstatus, t0
    la ra, park
    csrr	a0, sscratch
    sret


.section .text
.global park
.align 4
park:
    wfi
    j   park
park_end:

# void *idleproc(void);
# uint64_t idleprocsize;
.section .rodata
.global idleproc
.global idleprocsize
idleproc: .dword park
idleprocsize: .dword park_end - park
