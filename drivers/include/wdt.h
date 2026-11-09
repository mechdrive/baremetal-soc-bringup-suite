/**
 * @file wdt.h
 * @brief Hardware Watchdog Timer (WDT) Driver & Supervision Engine
 *
 * Provides watchdog arming, counter refreshing (petting/feeding),
 * timeout window configuration, and test-driven system reset triggers.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef WDT_H
#define WDT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t timeout_ms;
    bool enable_reset;
    bool enable_irq;
} wdt_config_t;

/**
 * @brief Initialize and arm the hardware watchdog timer.
 * @param base_addr MMIO base address
 * @param config Pointer to configuration struct
 * @return 0 on success, negative error code otherwise
 */
int wdt_init(uintptr_t base_addr, const wdt_config_t *config);

/**
 * @brief Feed / refresh the watchdog counter to prevent reset.
 * @param base_addr MMIO base address
 */
void wdt_feed(uintptr_t base_addr);

/**
 * @brief Get remaining count before watchdog triggers.
 * @param base_addr MMIO base address
 * @return Current counter value
 */
uint32_t wdt_get_current_count(uintptr_t base_addr);

/**
 * @brief Explicitly trigger watchdog timeout to test system reset behavior.
 * @param base_addr MMIO base address
 */
void wdt_force_reset_test(uintptr_t base_addr);

#ifdef __cplusplus
}
#endif

#endif /* WDT_H */

