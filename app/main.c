/**
 * @file main.c
 * @brief Bare-Metal Silicon Bringup Framework Orchestrator Entry Point
 *
 * Coordinates boot sequence, system hardware initialization, automated silicon
 * diagnostic regression suites, and transitions into interactive bringup monitor.
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "bringup_framework.h"
#include "diag_memory.h"
#include "diag_peripheral.h"
#include "diag_interrupt.h"
#include "diag_stress.h"
#include "bringup_cli.h"
#include "uart.h"
#include "timer.h"
#include "soc_regs.h"

static void early_hardware_init(void)
{
    /* 1. Configure Primary UART for high-speed diagnostic telemetry */
    uart_config_t uart_cfg = {
        .baudrate = 115200,
        .data_bits = 8,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOP_1,
        .enable_fifo = true,
        .loopback_mode = false
    };
    uart_init(SOC_UART0_BASE, &uart_cfg);

    /* 2. Configure System Core Timer (25 MHz reference) */
    timer_init(SOC_TIMER0_BASE, 25000000);
}

int main(void)
{
    /* Phase 0: Initialize Core Hardware Interfaces */
    early_hardware_init();

    /* Banner output */
    diag_log_raw("\r\n\r\n");
    diag_log_raw("================================================================================\r\n");
    diag_log_raw("  BARE-METAL ARM CORTEX-M / RISC-V SILICON BRINGUP & DIAGNOSTIC FRAMEWORK       \r\n");
    diag_log_raw("  Author: Poonam Singh | Silicon Qualification & Pre-Silicon Validation Suite   \r\n");
    diag_log_raw("================================================================================\r\n");

    /* Phase 1: Initialize Bringup Framework Registry */
    bringup_framework_init();

    /* Phase 2: Register All Diagnostic Test Suites */
    diag_memory_register_all();
    diag_peripheral_register_all();
    diag_interrupt_register_all();
    diag_stress_register_all();

    /* Phase 3: Execute Automated Silicon Validation Test Suite */
    diag_log(LOG_LEVEL_INFO, "Initiating Phase 1-7 Automated Silicon Bringup Qualification...");
    uint32_t fail_count = bringup_run_all();

    if (fail_count == 0) {
        diag_log(LOG_LEVEL_INFO, ">> Silicon validation complete: 100%% of diagnostics passed successfully <<");
    } else {
        diag_log(LOG_LEVEL_ERROR, ">> Silicon validation encountered %u diagnostic failure(s) <<", fail_count);
    }

    /* Phase 4: Launch Interactive Bringup Console */
    bringup_cli_run_interactive();

    /* Unreachable in bare-metal */
    while (1) {
        __asm__ volatile ("" ::: "memory");
    }

    return 0;
}
