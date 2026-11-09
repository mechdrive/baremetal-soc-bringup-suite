/**
 * @file diag_common.h
 * @brief Common Types, Enums & Status Codes for Bringup Suite
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef DIAG_COMMON_H
#define DIAG_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEST_RESULT_PASS = 0,
    TEST_RESULT_FAIL,
    TEST_RESULT_SKIPPED,
    TEST_RESULT_TIMEOUT,
    TEST_RESULT_CRASH
} test_result_t;

typedef enum {
    PHASE_EARLY_BOOT = 1,
    PHASE_CLOCK_POWER,
    PHASE_MEMORY_SRAM,
    PHASE_BUS_FABRIC,
    PHASE_PERIPHERALS,
    PHASE_INTERRUPTS,
    PHASE_STRESS_BURNIN
} bringup_phase_t;

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

typedef struct {
    uint32_t test_id;
    const char *name;
    const char *description;
    bringup_phase_t phase;
    test_result_t (*test_fn)(void);
    test_result_t result;
    uint32_t duration_us;
    uintptr_t fail_address;
    uint32_t expected_val;
    uint32_t actual_val;
    const char *fail_msg;
} test_case_t;

typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint32_t timeout_tests;
    uint64_t total_duration_us;
} bringup_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* DIAG_COMMON_H */
