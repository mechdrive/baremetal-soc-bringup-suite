/**
 * @file bringup_framework.c
 * @brief Bare-Metal Silicon Bringup Test Harness Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "bringup_framework.h"
#include "timer.h"
#include <stdio.h>
#include <string.h>

test_case_t *g_current_test = NULL;

static test_case_t s_tests[MAX_REGISTERED_TESTS];
static uint32_t s_num_tests = 0;
static bringup_stats_t s_stats;

void bringup_framework_init(void)
{
    s_num_tests = 0;
    memset(&s_stats, 0, sizeof(s_stats));
    memset(s_tests, 0, sizeof(s_tests));
    g_current_test = NULL;
}

int bringup_register_test(const test_case_t *test)
{
    if (!test || !test->test_fn) return -1;
    if (s_num_tests >= MAX_REGISTERED_TESTS) return -2;

    s_tests[s_num_tests] = *test;
    s_tests[s_num_tests].result = TEST_RESULT_SKIPPED;
    s_tests[s_num_tests].duration_us = 0;
    s_tests[s_num_tests].fail_msg = NULL;
    s_num_tests++;
    return 0;
}

static test_result_t execute_single(test_case_t *tc)
{
    g_current_test = tc;
    tc->fail_address = 0;
    tc->expected_val = 0;
    tc->actual_val = 0;
    tc->fail_msg = NULL;

    diag_log(LOG_LEVEL_INFO, "[START] [%02u] %-28s : %s", tc->test_id, tc->name, tc->description);

    uint64_t t_start = timer_get_time_us();
    test_result_t res = tc->test_fn();
    uint64_t t_end = timer_get_time_us();

    tc->result = res;
    tc->duration_us = (uint32_t)(t_end - t_start);

    s_stats.total_tests++;
    s_stats.total_duration_us += tc->duration_us;

    if (res == TEST_RESULT_PASS) {
        s_stats.passed_tests++;
        diag_log(LOG_LEVEL_INFO, "[PASS ] [%02u] %-28s (%lu us)", tc->test_id, tc->name, (unsigned long)tc->duration_us);
    } else if (res == TEST_RESULT_FAIL) {
        s_stats.failed_tests++;
        diag_log(LOG_LEVEL_ERROR, "[FAIL ] [%02u] %-28s (%lu us) - Reason: %s",
                 tc->test_id, tc->name, (unsigned long)tc->duration_us,
                 tc->fail_msg ? tc->fail_msg : "Assertion failed");
        if (tc->fail_address != 0) {
            diag_log(LOG_LEVEL_ERROR, "        Address: 0x%08lX | Expected: 0x%08lX | Actual: 0x%08lX",
                     (unsigned long)tc->fail_address, (unsigned long)tc->expected_val, (unsigned long)tc->actual_val);
        }
    } else if (res == TEST_RESULT_TIMEOUT) {
        s_stats.timeout_tests++;
        diag_log(LOG_LEVEL_WARN, "[TIMEO] [%02u] %-28s (%lu us)", tc->test_id, tc->name, (unsigned long)tc->duration_us);
    } else {
        s_stats.skipped_tests++;
        diag_log(LOG_LEVEL_WARN, "[SKIP ] [%02u] %-28s", tc->test_id, tc->name);
    }

    g_current_test = NULL;
    return res;
}

test_result_t bringup_run_test_by_id(uint32_t test_id)
{
    for (uint32_t i = 0; i < s_num_tests; ++i) {
        if (s_tests[i].test_id == test_id) {
            return execute_single(&s_tests[i]);
        }
    }
    diag_log(LOG_LEVEL_ERROR, "Test ID %u not found in registry", test_id);
    return TEST_RESULT_SKIPPED;
}

uint32_t bringup_run_phase(bringup_phase_t phase)
{
    uint32_t fails = 0;
    const char *phase_name = "UNKNOWN";
    switch (phase) {
        case PHASE_EARLY_BOOT:    phase_name = "PHASE 1: Early Boot & Power"; break;
        case PHASE_CLOCK_POWER:   phase_name = "PHASE 2: Clock Tree & PLL Lock"; break;
        case PHASE_MEMORY_SRAM:   phase_name = "PHASE 3: SRAM & Memory Subsystem"; break;
        case PHASE_BUS_FABRIC:    phase_name = "PHASE 4: Bus Fabric & Core MPU/PMP"; break;
        case PHASE_PERIPHERALS:   phase_name = "PHASE 5: Peripheral Subsystems"; break;
        case PHASE_INTERRUPTS:    phase_name = "PHASE 6: Interrupts & Exceptions"; break;
        case PHASE_STRESS_BURNIN: phase_name = "PHASE 7: Silicon Stress & Concurrent Load"; break;
    }

    diag_log_raw("\r\n================================================================================\r\n");
    diag_log(LOG_LEVEL_INFO, ">>>>> Executing %s <<<<<", phase_name);
    diag_log_raw("================================================================================\r\n");

    for (uint32_t i = 0; i < s_num_tests; ++i) {
        if (s_tests[i].phase == phase) {
            if (execute_single(&s_tests[i]) != TEST_RESULT_PASS) {
                fails++;
            }
        }
    }
    return fails;
}

uint32_t bringup_run_all(void)
{
    bringup_phase_t phases[] = {
        PHASE_EARLY_BOOT,
        PHASE_CLOCK_POWER,
        PHASE_MEMORY_SRAM,
        PHASE_BUS_FABRIC,
        PHASE_PERIPHERALS,
        PHASE_INTERRUPTS,
        PHASE_STRESS_BURNIN
    };

    memset(&s_stats, 0, sizeof(s_stats));
    uint32_t total_fails = 0;

    for (size_t i = 0; i < sizeof(phases)/sizeof(phases[0]); ++i) {
        total_fails += bringup_run_phase(phases[i]);
    }

    bringup_print_report();
    return total_fails;
}

void bringup_print_report(void)
{
    char buf[128];
    diag_log_raw("\r\n");
    diag_log_raw("################################################################################\r\n");
    diag_log_raw("#                   SILICON BRINGUP & DIAGNOSTIC REPORT                       #\r\n");
    diag_log_raw("################################################################################\r\n");
    diag_log_raw("| ID | Test Name                    | Result  | Time (us)  | Details           |\r\n");
    diag_log_raw("+----+------------------------------+---------+------------+-------------------+\r\n");

    for (uint32_t i = 0; i < s_num_tests; ++i) {
        const char *res_str = "SKIP";
        if (s_tests[i].result == TEST_RESULT_PASS) res_str = "PASS";
        else if (s_tests[i].result == TEST_RESULT_FAIL) res_str = "FAIL";
        else if (s_tests[i].result == TEST_RESULT_TIMEOUT) res_str = "TIMEO";

        snprintf(buf, sizeof(buf), "| %02u | %-28s | %-7s | %10lu | %-17s |\r\n",
                 (unsigned int)s_tests[i].test_id,
                 s_tests[i].name,
                 res_str,
                 (unsigned long)s_tests[i].duration_us,
                 s_tests[i].fail_msg ? s_tests[i].fail_msg : "OK");
        diag_log_raw(buf);
    }

    diag_log_raw("+----+------------------------------+---------+------------+-------------------+\r\n");
    snprintf(buf, sizeof(buf), "TOTAL: %lu | PASSED: %lu | FAILED: %lu | TIMEOUT: %lu | TIME: %lu us\r\n",
             (unsigned long)s_stats.total_tests,
             (unsigned long)s_stats.passed_tests,
             (unsigned long)s_stats.failed_tests,
             (unsigned long)s_stats.timeout_tests,
             (unsigned long)s_stats.total_duration_us);
    diag_log_raw(buf);

    if (s_stats.failed_tests == 0 && s_stats.passed_tests > 0) {
        diag_log_raw("[SILICON HEALTH] >>> ALL BRINGUP TESTS PASSED: SILICON REVISION QUALIFIED <<<\r\n");
    } else {
        diag_log_raw("[SILICON HEALTH] >>> WARNING: SILICON BRINGUP DEFECTS IDENTIFIED <<<\r\n");
    }
    diag_log_raw("################################################################################\r\n\r\n");
}

void bringup_get_stats(bringup_stats_t *stats)
{
    if (stats) {
        *stats = s_stats;
    }
}

