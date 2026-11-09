/**
 * @file diag_stress.h
 * @brief Silicon Stress, Concurrent Bus Traffic & Burn-In Test Declarations
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef DIAG_STRESS_H
#define DIAG_STRESS_H

#include "diag_common.h"

#ifdef __cplusplus
extern "C" {
#endif

test_result_t test_stress_concurrent_bus_traffic(void);
test_result_t test_stress_burst_bandwidth(void);

void diag_stress_register_all(void);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_STRESS_H */

