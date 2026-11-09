/**
 * @file sim_main.c
 * @brief Host Silicon Bringup Simulation Harness
 *
 * Runs the complete bare-metal bringup suite and test harness natively on host OS
 * to enable automated regression testing and validation without target hardware.
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "sim_soc.h"
#include "bringup_framework.h"
#include "diag_memory.h"
#include "diag_peripheral.h"
#include "diag_interrupt.h"
#include "diag_stress.h"
#include "bringup_cli.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    /* Initialize Virtual Silicon Bus */
    sim_soc_init();

    printf("\n");
    printf("================================================================================\n");
    printf("  VIRTUAL SILICON BRINGUP & HARDWARE-IN-THE-LOOP (HIL) SIMULATION TESTBENCH     \n");
    printf("  Author: Poonam Singh | Pre-Silicon Qualification & Regression Verification    \n");
    printf("================================================================================\n\n");

    /* Initialize Framework */
    bringup_framework_init();

    /* Register Test Suites */
    diag_memory_register_all();
    diag_peripheral_register_all();
    diag_interrupt_register_all();
    diag_stress_register_all();

    /* Run Full Regression */
    printf("[SIMULATOR] Launching automated bringup phases 1 through 7...\n");
    uint32_t fail_count = bringup_run_all();

    /* Execute sample CLI commands to test command processor */
    printf("\n[SIMULATOR] Verifying Silicon Bringup CLI Command Interpreter...\n");
    char cmd1[] = "version";
    bringup_cli_execute_command(cmd1);

    char cmd2[] = "status";
    bringup_cli_execute_command(cmd2);

    char cmd3[] = "peek 0x40004018"; /* Peek UART FR */
    bringup_cli_execute_command(cmd3);

    char cmd4[] = "mtest 0x20010000 256 all";
    bringup_cli_execute_command(cmd4);

    bringup_stats_t stats;
    bringup_get_stats(&stats);

    printf("\n================================================================================\n");
    printf("  SIMULATION REGRESSION COMPLETE: %u PASSED, %u FAILED (%lu us total runtime)   \n",
           stats.passed_tests, stats.failed_tests, (unsigned long)stats.total_duration_us);
    printf("================================================================================\n\n");

    sim_soc_destroy();
    return (fail_count == 0) ? 0 : 1;
}
