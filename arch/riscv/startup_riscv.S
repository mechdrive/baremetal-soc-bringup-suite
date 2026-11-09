/**
 * @file startup_riscv.S
 * @brief RISC-V RV32 Assembly Boot Entry & Trap Vector
 *
 * @author Poonam Singh
 * @date 2026
 */

    .section .text.init
    .global _start
    .type _start, @function

_start:
    /* Disable interrupts during early bringup */
    csrw    mie, zero

    /* Set up stack pointer (stack grows downwards from _stack_top) */
    la      sp, _stack_top

    /* Set trap vector handler */
    la      t0, trap_vector_entry
    csrw    mtvec, t0

    /* Copy initialized .data section from ROM/LMA to RAM/VMA */
    la      a0, _sdata
    la      a1, _edata
    la      a2, _sidata
    bge     a0, a1, 2f
1:
    lw      t0, 0(a2)
    sw      t0, 0(a0)
    addi    a0, a0, 4
    addi    a2, a2, 4
    blt     a0, a1, 1b
2:

    /* Clear uninitialized .bss section to zero */
    la      a0, _sbss
    la      a1, _ebss
    bge     a0, a1, 4f
3:
    sw      zero, 0(a0)
    addi    a0, a0, 4
    blt     a0, a1, 3b
4:

    /* Call main application */
    call    main

1:
    wfi
    j       1b

    .size _start, .-_start

    /* Trap Vector Trampoline */
    .align 4
    .global trap_vector_entry
    .type trap_vector_entry, @function
trap_vector_entry:
    /* Allocate space on stack for riscv_trap_frame_t (19 * 4 = 76 bytes) */
    addi    sp, sp, -76
    sw      ra, 0(sp)
    sw      t0, 4(sp)
    sw      t1, 8(sp)
    sw      t2, 12(sp)
    sw      a0, 16(sp)
    sw      a1, 20(sp)
    sw      a2, 24(sp)
    sw      a3, 28(sp)
    sw      a4, 32(sp)
    sw      a5, 36(sp)
    sw      a6, 40(sp)
    sw      a7, 44(sp)
    sw      t3, 48(sp)
    sw      t4, 52(sp)
    sw      t5, 56(sp)
    sw      t6, 60(sp)

    csrr    t0, mepc
    sw      t0, 64(sp)
    csrr    t1, mcause
    sw      t1, 68(sp)
    csrr    t2, mtval
    sw      t2, 72(sp)

    /* Pass trap frame pointer in a0 */
    mv      a0, sp
    call    riscv_trap_handler_c

    /* Restore registers */
    lw      t0, 64(sp)
    csrw    mepc, t0

    lw      ra, 0(sp)
    lw      t0, 4(sp)
    lw      t1, 8(sp)
    lw      t2, 12(sp)
    lw      a0, 16(sp)
    lw      a1, 20(sp)
    lw      a2, 24(sp)
    lw      a3, 28(sp)
    lw      a4, 32(sp)
    lw      a5, 36(sp)
    lw      a6, 40(sp)
    lw      a7, 44(sp)
    lw      t3, 48(sp)
    lw      t4, 52(sp)
    lw      t5, 56(sp)
    lw      t6, 60(sp)
    addi    sp, sp, 76

    mret

    .size trap_vector_entry, .-trap_vector_entry
