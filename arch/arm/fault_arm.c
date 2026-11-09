/**
 * @file fault_arm.c
 * @brief ARM Cortex-M HardFault & Exception Diagnostics Unwinder
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "cpu_arm.h"
#include "bringup_framework.h"
#include <stdio.h>

void arm_fault_handler_c(arm_exception_frame_t *frame, uint32_t lr_exc)
{
    char buf[128];
    diag_log_raw("\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    diag_log_raw("                      *** ARM CORTEX-M HARDFAULT EXCEPTION ***                  \r\n");
    diag_log_raw("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

    snprintf(buf, sizeof(buf), "EXC_RETURN (LR) : 0x%08lX (%s stack used)\r\n",
             (unsigned long)lr_exc, (lr_exc & 0x4) ? "Process (PSP)" : "Main (MSP)");
    diag_log_raw(buf);

    snprintf(buf, sizeof(buf), "R0  : 0x%08lX  | R1  : 0x%08lX  | R2  : 0x%08lX  | R3 : 0x%08lX\r\n",
             (unsigned long)frame->r0, (unsigned long)frame->r1,
             (unsigned long)frame->r2, (unsigned long)frame->r3);
    diag_log_raw(buf);

    snprintf(buf, sizeof(buf), "R12 : 0x%08lX  | LR  : 0x%08lX  | PC  : 0x%08lX  | PSR: 0x%08lX\r\n",
             (unsigned long)frame->r12, (unsigned long)frame->lr,
             (unsigned long)frame->pc, (unsigned long)frame->xpsr);
    diag_log_raw(buf);

    uint32_t cfsr = SCB_CFSR;
    uint32_t hfsr = SCB_HFSR;
    snprintf(buf, sizeof(buf), "CFSR: 0x%08lX  | HFSR: 0x%08lX\r\n", (unsigned long)cfsr, (unsigned long)hfsr);
    diag_log_raw(buf);

    /* Decode Usage Faults (upper 16 bits of CFSR) */
    uint16_t ufsr = (uint16_t)(cfsr >> 16);
    if (ufsr) {
        diag_log_raw("[CAUSE: USAGE FAULT] ");
        if (ufsr & (1 << 0)) diag_log_raw("Undefined Instruction | ");
        if (ufsr & (1 << 1)) diag_log_raw("Invalid State (EPSC/T-bit) | ");
        if (ufsr & (1 << 2)) diag_log_raw("Invalid PC Load | ");
        if (ufsr & (1 << 3)) diag_log_raw("No Coprocessor | ");
        if (ufsr & (1 << 8)) diag_log_raw("Unaligned Access | ");
        if (ufsr & (1 << 9)) diag_log_raw("Divide By Zero | ");
        diag_log_raw("\r\n");
    }

    /* Decode Bus Faults (middle 8 bits of CFSR) */
    uint8_t bfsr = (uint8_t)((cfsr >> 8) & 0xFF);
    if (bfsr) {
        diag_log_raw("[CAUSE: BUS FAULT] ");
        if (bfsr & (1 << 0)) diag_log_raw("Instruction Bus Error | ");
        if (bfsr & (1 << 1)) diag_log_raw("Precise Data Bus Error | ");
        if (bfsr & (1 << 2)) diag_log_raw("Imprecise Data Bus Error | ");
        if (bfsr & (1 << 7)) {
            snprintf(buf, sizeof(buf), "Faulting Bus Address: 0x%08lX | ", (unsigned long)SCB_BFAR);
            diag_log_raw(buf);
        }
        diag_log_raw("\r\n");
    }

    /* Decode Memory Management Faults (lower 8 bits of CFSR) */
    uint8_t mmfsr = (uint8_t)(cfsr & 0xFF);
    if (mmfsr) {
        diag_log_raw("[CAUSE: MEMMANAGE FAULT] ");
        if (mmfsr & (1 << 0)) diag_log_raw("Instruction Access Violation | ");
        if (mmfsr & (1 << 1)) diag_log_raw("Data Access Violation | ");
        if (mmfsr & (1 << 7)) {
            snprintf(buf, sizeof(buf), "Faulting MPU Address: 0x%08lX | ", (unsigned long)SCB_MMFAR);
            diag_log_raw(buf);
        }
        diag_log_raw("\r\n");
    }

    diag_log_raw("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    diag_log_raw("Halting core. Attach GDB to inspect state.\r\n");

    while (1) {
        __asm__ volatile ("bkpt #0");
    }
}
