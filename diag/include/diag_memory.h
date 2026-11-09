/**
 * @file diag_memory.h
 * @brief Memory Subsystem Diagnostic Test Declarations
 *
 * Covers Walking 1s/0s, Address Bus Aliasing, March C-, and LFSR Random Patterns.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef DIAG_MEMORY_H
#define DIAG_MEMORY_H

#include "diag_common.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Algorithm execution primitives */
test_result_t diag_mem_walking_ones(uintptr_t base_addr);
test_result_t diag_mem_walking_zeros(uintptr_t base_addr);
test_result_t diag_mem_address_bus_integrity(uintptr_t base_addr, size_t size_bytes);
test_result_t diag_mem_march_c_minus(uintptr_t base_addr, size_t num_words);
test_result_t diag_mem_lfsr_pattern(uintptr_t base_addr, size_t num_words, uint32_t seed);

/* Test suite runners for framework registration */
test_result_t test_mem_walking_ones(void);
test_result_t test_mem_walking_zeros(void);
test_result_t test_mem_address_bus_integrity(void);
test_result_t test_mem_march_c_minus(void);
test_result_t test_mem_lfsr_pattern(void);

void diag_memory_register_all(void);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_MEMORY_H */
