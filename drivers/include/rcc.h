/**
 * @file rcc.h
 * @brief Reset and Clock Control (RCC) & Silicon Power Manager
 *
 * Manages oscillator sources (HSI, HSE), Phase-Locked Loop (PLL) configuration,
 * system clock switching, peripheral clock distribution, and reset reason capture.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef RCC_H
#define RCC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLK_SRC_HSI = 0, /* Internal High-Speed RC Oscillator (typically 16MHz) */
    CLK_SRC_HSE = 1, /* External Crystal Oscillator */
    CLK_SRC_PLL = 2  /* Phase-Locked Loop Multiplied Clock */
} clock_source_t;

typedef enum {
    RESET_REASON_UNKNOWN = 0,
    RESET_REASON_POR,        /* Power-on / Power-down reset */
    RESET_REASON_PIN,        /* External NRST pin pull */
    RESET_REASON_SOFTWARE,   /* Software system reset request */
    RESET_REASON_IWDG,       /* Independent watchdog timeout */
    RESET_REASON_WWDG,       /* Window watchdog timeout */
    RESET_REASON_LOW_POWER,  /* Low-power management reset */
    RESET_REASON_BROWNOUT    /* Brown-out voltage drop reset */
} reset_reason_t;

typedef struct {
    clock_source_t target_source;
    uint32_t pll_multiplier;
    uint32_t pll_prediv;
    uint32_t ahb_prescaler;
    uint32_t apb1_prescaler;
    uint32_t apb2_prescaler;
} clock_tree_config_t;

/**
 * @brief Initialize clock tree and lock PLL with timeout safeguards.
 * @param base_addr MMIO base address of RCC peripheral
 * @param config Pointer to clock configuration
 * @param timeout_loops Loop timeout for PLL lock
 * @return 0 on success, negative error code if oscillator/PLL fails to lock
 */
int rcc_init_clocks(uintptr_t base_addr, const clock_tree_config_t *config, uint32_t timeout_loops);

/**
 * @brief Inspect hardware flags to identify cause of latest system reset.
 * @param base_addr MMIO base address of RCC
 * @return Decoded reset reason enum
 */
reset_reason_t rcc_get_reset_reason(uintptr_t base_addr);

/**
 * @brief Convert reset reason enum to human-readable string.
 * @param reason Reset reason enum
 * @return Const string description
 */
const char *rcc_reset_reason_to_str(reset_reason_t reason);

/**
 * @brief Clear sticky reset reason flags in RCC status register.
 * @param base_addr MMIO base address
 */
void rcc_clear_reset_flags(uintptr_t base_addr);

/**
 * @brief Enable clock gating for specific peripheral bus device.
 * @param base_addr MMIO base address
 * @param bus_reg_offset Offset to AHBENR / APB1ENR / APB2ENR
 * @param periph_mask Bitmask of peripheral to enable
 */
void rcc_enable_peripheral_clock(uintptr_t base_addr, uint32_t bus_reg_offset, uint32_t periph_mask);

/**
 * @brief Disable clock gating for specific peripheral bus device.
 * @param base_addr MMIO base address
 * @param bus_reg_offset Offset to AHBENR / APB1ENR / APB2ENR
 * @param periph_mask Bitmask of peripheral to disable
 */
void rcc_disable_peripheral_clock(uintptr_t base_addr, uint32_t bus_reg_offset, uint32_t periph_mask);

#ifdef __cplusplus
}
#endif

#endif /* RCC_H */

