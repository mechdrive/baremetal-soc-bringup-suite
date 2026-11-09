/**
 * @file sim_soc.h
 * @brief Virtual Silicon Bus & Peripheral Simulation Engine Header
 *
 * Implements a cycle-accurate virtual SoC MMIO bus for pre-silicon verification
 * and host testing without physical hardware or target emulators.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef SIM_SOC_H
#define SIM_SOC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the virtual SoC simulation bus and allocate peripheral register spaces.
 */
void sim_soc_init(void);

/**
 * @brief Shutdown virtual SoC bus and release resources.
 */
void sim_soc_destroy(void);

/**
 * @brief Step the virtual peripheral clocks and tick state machines.
 * @param cycles Number of clock cycles to advance
 */
void sim_soc_step_clocks(uint32_t cycles);

/**
 * @brief Inject a simulated hardware interrupt into the virtual NVIC / PLIC.
 * @param irq_num Interrupt line number
 */
void sim_soc_inject_irq(uint32_t irq_num);

#ifdef __cplusplus
}
#endif

#endif /* SIM_SOC_H */
