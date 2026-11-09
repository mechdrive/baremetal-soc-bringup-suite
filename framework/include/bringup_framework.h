/**
 * @file bringup_framework.h
 * @brief Bare-Metal Silicon Bringup Test Harness & Assertion Engine
 *
 * Provides structured test execution, timing measurements, register assertions,
 * and automated failure diagnosis for no-OS environments.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef BRINGUP_FRAMEWORK_H
#define BRINGUP_FRAMEWORK_H

#include "diag_common.h"
#include "mmio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_REGISTERED_TESTS 64

/* Current active test tracking for assertion macros */
extern test_case_t *g_current_test;

/* -------------------------------------------------------------------------- */
/* Assertion Macros                                                           */
/* -------------------------------------------------------------------------- */

#define TEST_PASS() return TEST_RESULT_PASS

#define TEST_FAIL(msg) do { \
    if (g_current_test) { \
        g_current_test->fail_msg = (msg); \
    } \
    return TEST_RESULT_FAIL; \
} while (0)

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        if (g_current_test) { \
            g_current_test->fail_msg = (msg); \
        } \
        return TEST_RESULT_FAIL; \
    } \
} while (0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)

#define ASSERT_EQ(exp, act, msg) do { \
    uint32_t _e = (uint32_t)(exp); \
    uint32_t _a = (uint32_t)(act); \
    if (_e != _a) { \
        if (g_current_test) { \
            g_current_test->expected_val = _e; \
            g_current_test->actual_val = _a; \
            g_current_test->fail_msg = (msg); \
        } \
        return TEST_RESULT_FAIL; \
    } \
} while (0)

#define ASSERT_REG_EQ(addr, exp, msg) do { \
    uint32_t _e = (uint32_t)(exp); \
    uint32_t _a = mmio_read32(addr); \
    if (_e != _a) { \
        if (g_current_test) { \
            g_current_test->fail_address = (uintptr_t)(addr); \
            g_current_test->expected_val = _e; \
            g_current_test->actual_val = _a; \
            g_current_test->fail_msg = (msg); \
        } \
        return TEST_RESULT_FAIL; \
    } \
} while (0)

#define ASSERT_REG_BITS_SET(addr, mask, msg) do { \
    uint32_t _m = (uint32_t)(mask); \
    uint32_t _a = mmio_read32(addr); \
    if ((_a & _m) != _m) { \
        if (g_current_test) { \
            g_current_test->fail_address = (uintptr_t)(addr); \
            g_current_test->expected_val = _m; \
            g_current_test->actual_val = (_a & _m); \
            g_current_test->fail_msg = (msg); \
        } \
        return TEST_RESULT_FAIL; \
    } \
} while (0)

#define ASSERT_REG_BITS_CLR(addr, mask, msg) do { \
    uint32_t _m = (uint32_t)(mask); \
    uint32_t _a = mmio_read32(addr); \
    if ((_a & _m) != 0) { \
        if (g_current_test) { \
            g_current_test->fail_address = (uintptr_t)(addr); \
            g_current_test->expected_val = 0; \
            g_current_test->actual_val = (_a & _m); \
            g_current_test->fail_msg = (msg); \
        } \
        return TEST_RESULT_FAIL; \
    } \
} while (0)

/* -------------------------------------------------------------------------- */
/* Framework Management APIs                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the silicon bringup framework and test table.
 */
void bringup_framework_init(void);

/**
 * @brief Register a test case into the bringup test suite.
 * @param test Pointer to populated test_case_t
 * @return 0 on success, negative if test table is full
 */
int bringup_register_test(const test_case_t *test);

/**
 * @brief Run a single test case by its ID.
 * @param test_id Unique numeric ID of test
 * @return test_result_t result
 */
test_result_t bringup_run_test_by_id(uint32_t test_id);

/**
 * @brief Run all tests assigned to a specific bringup phase.
 * @param phase Phase enum
 * @return Number of failed tests in phase
 */
uint32_t bringup_run_phase(bringup_phase_t phase);

/**
 * @brief Run all registered test cases sequentially.
 * @return Total number of failed tests
 */
uint32_t bringup_run_all(void);

/**
 * @brief Print comprehensive diagnostic results table and silicon health summary.
 */
void bringup_print_report(void);

/**
 * @brief Get copy of current execution statistics.
 * @param stats Pointer to destination stats struct
 */
void bringup_get_stats(bringup_stats_t *stats);

/* -------------------------------------------------------------------------- */
/* Logging Primitives                                                         */
/* -------------------------------------------------------------------------- */

void diag_log(log_level_t level, const char *fmt, ...);
void diag_log_raw(const char *str);
void diag_dump_hex(const char *tag, uintptr_t addr, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BRINGUP_FRAMEWORK_H */

