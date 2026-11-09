/**
 * @file cpu_riscv.h
 * @brief RISC-V RV32 Control and Status Registers (CSR) & Trap Definitions
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef CPU_RISCV_H
#define CPU_RISCV_H

#include <stdint.h>

/* CSR Read / Write Inline Macros */
#define csr_read(csr) ({ \
    register uint32_t __v; \
    __asm__ volatile ("csrr %0, " #csr : "=r" (__v) : : "memory"); \
    __v; })

#define csr_write(csr, val) ({ \
    register uint32_t __v = (uint32_t)(val); \
    __asm__ volatile ("csrw " #csr ", %0" : : "rK" (__v) : "memory"); })

#define csr_set(csr, val) ({ \
    register uint32_t __v = (uint32_t)(val); \
    __asm__ volatile ("csrs " #csr ", %0" : : "rK" (__v) : "memory"); })

#define csr_clear(csr, val) ({ \
    register uint32_t __v = (uint32_t)(val); \
    __asm__ volatile ("csrc " #csr ", %0" : : "rK" (__v) : "memory"); })

/* Standard CSR Register Numbers / Names */
#define CSR_MSTATUS     mstatus
#define CSR_MIE         mie
#define CSR_MTVEC       mtvec
#define CSR_MEPC        mepc
#define CSR_MCAUSE      mcause
#define CSR_MTVAL       mtval
#define CSR_MIP         mip

/* Trap Frame on RISC-V Trap Entry */
typedef struct {
    uint32_t ra;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t mepc;
    uint32_t mcause;
    uint32_t mtval;
} riscv_trap_frame_t;

void riscv_trap_handler_c(riscv_trap_frame_t *frame);

#endif /* CPU_RISCV_H */
