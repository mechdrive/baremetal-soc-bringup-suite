/**
 * @file diag_interrupt.h
 * @brief Interrupt Controller (NVIC / PLIC) & Exception Diagnostic Declarations
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef DIAG_INTERRUPT_H
#define DIAG_INTERRUPT_H

#include "diag_common.h"

#ifdef __cplusplus
extern "C" {
#endif

test_result_t test_interrupt_controller_registers(void);
test_result_t test_interrupt_software_trigger(void);
test_result_t test_interrupt_latency_profile(void);
test_result_t test_core_mpu_protection(void);

void diag_interrupt_register_all(void);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_INTERRUPT_H */

