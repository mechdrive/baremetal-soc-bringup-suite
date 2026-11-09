/**
 * @file trap_riscv.c
 * @brief RISC-V Machine-Mode Trap & Exception Unwinder
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "cpu_riscv.h"
#include "bringup_framework.h"
#include <stdio.h>

void riscv_trap_handler_c(riscv_trap_frame_t *frame)
{
    char buf[128];
    uint32_t mcause = frame->mcause;
    bool is_interrupt = (mcause & 0x80000000UL) != 0;
    uint32_t code = mcause & 0x7FFFFFFFUL;

    if (is_interrupt) {
        /* Handle Interrupts */
        switch (code) {
            case 3:  /* Machine Software Interrupt */
                diag_log(LOG_LEVEL_INFO, "[RISC-V IRQ] Machine Software Interrupt Handled");
                break;
            case 7:  /* Machine Timer Interrupt */
                diag_log(LOG_LEVEL_INFO, "[RISC-V IRQ] Machine Timer Interrupt Handled");
                break;
            case 11: /* Machine External Interrupt (PLIC) */
                diag_log(LOG_LEVEL_INFO, "[RISC-V IRQ] Machine External PLIC Interrupt Handled");
                break;
            default:
                diag_log(LOG_LEVEL_WARN, "[RISC-V IRQ] Unknown Interrupt: code %lu", (unsigned long)code);
                break;
        }
        return;
    }

    /* Handle Synchronous Exceptions / Faults */
    diag_log_raw("\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    diag_log_raw("                       *** RISC-V MACHINE-MODE TRAP ***                         \r\n");
    diag_log_raw("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");

    snprintf(buf, sizeof(buf), "MEPC   : 0x%08lX  | MCAUSE : 0x%08lX  | MTVAL : 0x%08lX\r\n",
             (unsigned long)frame->mepc, (unsigned long)frame->mcause, (unsigned long)frame->mtval);
    diag_log_raw(buf);

    snprintf(buf, sizeof(buf), "RA     : 0x%08lX  | A0     : 0x%08lX  | A1    : 0x%08lX\r\n",
             (unsigned long)frame->ra, (unsigned long)frame->a0, (unsigned long)frame->a1);
    diag_log_raw(buf);

    const char *desc = "Unknown Exception";
    switch (code) {
        case 0: desc = "Instruction Address Misaligned"; break;
        case 1: desc = "Instruction Access Fault"; break;
        case 2: desc = "Illegal Instruction"; break;
        case 3: desc = "Breakpoint (EBREAK)"; break;
        case 4: desc = "Load Address Misaligned"; break;
        case 5: desc = "Load Access Fault"; break;
        case 6: desc = "Store/AMO Address Misaligned"; break;
        case 7: desc = "Store/AMO Access Fault"; break;
        case 8: desc = "Environment Call from U-mode (ECALL)"; break;
        case 9: desc = "Environment Call from S-mode (ECALL)"; break;
        case 11: desc = "Environment Call from M-mode (ECALL)"; break;
    }

    snprintf(buf, sizeof(buf), "EXC TYPE: [%lu] %s\r\n", (unsigned long)code, desc);
    diag_log_raw(buf);

    diag_log_raw("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    diag_log_raw("Core halted. Connect with GDB to debug trap frame.\r\n");

    while (1) {
        __asm__ volatile ("wfi");
    }
}

