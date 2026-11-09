/**
 * @file cpu_arm.h
 * @brief ARM Cortex-M Architectural Definitions & Fault Status Registers
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef CPU_ARM_H
#define CPU_ARM_H

#include <stdint.h>

/* System Control Block (SCB) Base: 0xE000ED00 */
#define SCB_CPUID       (*(volatile uint32_t *)0xE000ED00)
#define SCB_ICSR        (*(volatile uint32_t *)0xE000ED04)
#define SCB_VTOR        (*(volatile uint32_t *)0xE000ED08)
#define SCB_AIRCR       (*(volatile uint32_t *)0xE000ED0C)
#define SCB_SCR         (*(volatile uint32_t *)0xE000ED10)
#define SCB_CCR         (*(volatile uint32_t *)0xE000ED14)
#define SCB_SHPR1       (*(volatile uint32_t *)0xE000ED18)
#define SCB_SHPR2       (*(volatile uint32_t *)0xE000ED1C)
#define SCB_SHPR3       (*(volatile uint32_t *)0xE000ED20)
#define SCB_SHCSR       (*(volatile uint32_t *)0xE000ED24)
#define SCB_CFSR        (*(volatile uint32_t *)0xE000ED28)
#define SCB_HFSR        (*(volatile uint32_t *)0xE000ED2C)
#define SCB_DFSR        (*(volatile uint32_t *)0xE000ED30)
#define SCB_MMFAR       (*(volatile uint32_t *)0xE000ED34)
#define SCB_BFAR        (*(volatile uint32_t *)0xE000ED38)

/* CFSR sub-registers */
#define SCB_CFSR_MMFSR_MASK 0x000000FFUL
#define SCB_CFSR_BFSR_MASK  0x0000FF00UL
#define SCB_CFSR_UFSR_MASK  0xFFFF0000UL

/* Stack frame saved on exception entry */
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} arm_exception_frame_t;

void arm_fault_handler_c(arm_exception_frame_t *frame, uint32_t lr_exc);

#endif /* CPU_ARM_H */

