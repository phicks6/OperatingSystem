.altmacro
.macro loadgp  i, r=t6
	ld	x\i, (\i * 8)(\r)
.endm

.macro savegp  i, r=t6
    sd  x\i, (\i * 8)(\r)
.endm

.macro loadfp  i, r=t6
	fld f\i, (\i * 8 + 256)(\r)
.endm

.macro savefp  i, r=t6
	fsd f\i, (\i * 8 + 256)(\r)
.endm



.section .text
_spawn_kthread:
    
    csrr        t6, sscratch
	# Spawn table fields
	# 0    - gpregs
    # 256  - fpregs
    # 512  - sepc
    # 520  - sstatus
    # 528  - sie
    # 536  - satp
    # 544  - sscratch
    # 552  - stvec
    # 560  - trap_satp
    # 568  - trap_stack

    //load sepc so we know what to go to after srett
	ld		t0, 512(t6)   # sepc
	csrw	sepc, t0

    //Read sstatus to be used later
	ld		t4, 520(t6)   # sstatus
	csrw	sstatus, t4

    //Load interupt info
	ld		t0, 528(t6)  # sie
	csrw	sie, t0

    //Load location of trap handler
	ld		t0, 552(t6)  # stvec
	csrw	stvec, t0

    //Load process page table
	ld		t1, 536(t6)  # satp

    //load sscratch 
	ld		t6, 544(t6)  # sscratch
	csrw	sscratch, t6

    #turn on the MMU
	csrw	satp, t1

    
    //Read sstatus and conditionally load fp registers 
    srli    t4, t4, 13
    andi    t4, t4, 3
    beqz    t4, 1f

    .set i, 0
    .rept 32
        loadfp %i
        .set i, i+1
    .endr

1:
	.set i, 1
	.rept 31
		loadgp  %i
		.set i, i + 1
	.endr

	

    

    
    

	

    
    //Go to what we put in sepc
	sret

.align 4
_spawn_kthread_end:


_spawn_trap:
    
    csrrw   t6, sscratch, t6

    //Save registers
    .set i, 1
    .rept 30
        savegp  %i
    .set i, i + 1
    .endr

    mv      t5, t6
    csrrw   t6, sscratch, t6
    savegp  31, t5

    //Read sstatus and conditionally do many things?
    csrr    t6, sstatus
    srli    t6, t6, 13
    beqz    t6, 1f

    csrr    t6, sscratch
    
    .set i, 0
    .rept 32
        savefp %i
        .set i, i + 1
    .endr

    ld      t5, 560(t6)
    ld      sp, 568(t6)
    csrw    satp, t5

    call    os_trap_handler

    //Load kernal page table
    csrr    t6, sscratch
    ld      t5, 536(t6) # satp
    csrw    satp, t5

    //Read sstatus and conditionally load fp registers 
    csrr    t5, sstatus
    srli    t5, t5, 13
    andi    t5, t5, 3
    beqz    t5, 1f
    .set i, 0
    .rept 32
        loadfp  %i
    .set i, i + 1
    .endr
1:
    .set i, 1
    .rept 31
        loadgp %i
    .set i, i + 1
    .endr
1:
    sret
_spawn_trap_end:



.align 4
.global _asm_trap_vector
_asm_trap_vector:
    
    csrrw t6, sscratch, t6

    #Save context
    .set i, 1
    .rept 30
        savegp %i
        .set i, i+1
    .endr

    #Saves last register
    mv t5, t6
    csrrw t6, sscratch, t6
    sd t6, 8*31(t5)
    
    #Switch to our C code
    //la  sp, _stack_end
    call    os_trap_handler
    csrr    t6, sscratch

    #Load context after doing stuff in c
    .set i, 1
    .rept 31
        loadgp %i
        .set i, i+1
    .endr

    sret
.align 4
_asm_trap_vector_end:


.section .rodata
.global spawn_thread_start
.global spawn_thread_end
.global spawn_thread_size
spawn_thread_start: .dword _spawn_kthread
spawn_thread_end: .dword _spawn_kthread_end
spawn_thread_size: .dword _spawn_kthread_end - _spawn_kthread

.global spawn_trap_start
.global spawn_trap_end
.global spawn_trap_size
spawn_trap_start: .dword _spawn_trap
spawn_trap_end: .dword _spawn_trap_end
spawn_trap_size: .dword _spawn_trap_end - _spawn_trap

