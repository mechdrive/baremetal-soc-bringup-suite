/**
 * @file rcc.c
 * @brief Reset and Clock Control (RCC) Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "rcc.h"
#include "mmio.h"
#include "soc_regs.h"

int rcc_init_clocks(uintptr_t base_addr, const clock_tree_config_t *config, uint32_t timeout_loops)
{
    if (base_addr == 0 || config == 0) return -1;

    /* 1. Turn on HSI internal RC oscillator */
    mmio_set_bits32(base_addr + RCC_CR_OFFSET, RCC_CR_HSION);
    if (!mmio_poll_bits_set(base_addr + RCC_CR_OFFSET, RCC_CR_HSIRDY, timeout_loops)) {
        return -2; /* HSI failed to stabilize */
    }

    /* 2. If HSE requested, enable external crystal */
    if (config->target_source == CLK_SRC_HSE || config->target_source == CLK_SRC_PLL) {
        mmio_set_bits32(base_addr + RCC_CR_OFFSET, RCC_CR_HSEON);
        if (!mmio_poll_bits_set(base_addr + RCC_CR_OFFSET, RCC_CR_HSERDY, timeout_loops)) {
            return -3; /* HSE failed to stabilize */
        }
    }

    /* 3. Configure PLL if requested */
    if (config->target_source == CLK_SRC_PLL) {
        /* Disable PLL before configuring */
        mmio_clear_bits32(base_addr + RCC_CR_OFFSET, RCC_CR_PLLON);
        if (!mmio_poll_bits_cleared(base_addr + RCC_CR_OFFSET, RCC_CR_PLLRDY, timeout_loops)) {
            return -4;
        }

        /* Write PLL multiplier & pre-divider */
        uint32_t pllcfgr = (config->pll_multiplier << 6) | (config->pll_prediv & 0x3F);
        mmio_write32(base_addr + RCC_PLLCFGR_OFFSET, pllcfgr);

        /* Turn on PLL */
        mmio_set_bits32(base_addr + RCC_CR_OFFSET, RCC_CR_PLLON);
        if (!mmio_poll_bits_set(base_addr + RCC_CR_OFFSET, RCC_CR_PLLRDY, timeout_loops)) {
            return -5; /* PLL failed to lock */
        }
    }

    /* 4. Switch system clock source */
    uint32_t sw_bits = 0;
    if (config->target_source == CLK_SRC_HSE) sw_bits = 1;
    else if (config->target_source == CLK_SRC_PLL) sw_bits = 2;

    mmio_modify32(base_addr + RCC_CFGR_OFFSET, 0x03, sw_bits);

    /* 5. Set AHB, APB1, APB2 prescalers */
    uint32_t cfgr = (config->ahb_prescaler << 4) | (config->apb1_prescaler << 10) | (config->apb2_prescaler << 13);
    mmio_modify32(base_addr + RCC_CFGR_OFFSET, 0x0000FFFC, cfgr);

    return 0;
}

reset_reason_t rcc_get_reset_reason(uintptr_t base_addr)
{
    if (base_addr == 0) return RESET_REASON_UNKNOWN;

    uint32_t csr = mmio_read32(base_addr + RCC_CSR_OFFSET);

    if (csr & RCC_CSR_LPWRRSTF) return RESET_REASON_LOW_POWER;
    if (csr & RCC_CSR_WWDGRSTF) return RESET_REASON_WWDG;
    if (csr & RCC_CSR_IWDGRSTF) return RESET_REASON_IWDG;
    if (csr & RCC_CSR_SFTRSTF)  return RESET_REASON_SOFTWARE;
    if (csr & RCC_CSR_PORRSTF)  return RESET_REASON_POR;
    if (csr & RCC_CSR_PINRSTF)  return RESET_REASON_PIN;
    if (csr & RCC_CSR_BORRSTF)  return RESET_REASON_BROWNOUT;

    return RESET_REASON_UNKNOWN;
}

const char *rcc_reset_reason_to_str(reset_reason_t reason)
{
    switch (reason) {
        case RESET_REASON_POR:       return "Power-On Reset (POR)";
        case RESET_REASON_PIN:       return "NRST Pin Reset";
        case RESET_REASON_SOFTWARE:  return "Software Reset Request";
        case RESET_REASON_IWDG:      return "Independent Watchdog (IWDG) Reset";
        case RESET_REASON_WWDG:      return "Window Watchdog (WWDG) Reset";
        case RESET_REASON_LOW_POWER: return "Low-Power Management Reset";
        case RESET_REASON_BROWNOUT:  return "Brown-Out Voltage Reset (BOR)";
        case RESET_REASON_UNKNOWN:
        default:                     return "Unknown / Undefined Reset";
    }
}

void rcc_clear_reset_flags(uintptr_t base_addr)
{
    if (base_addr != 0) {
        mmio_set_bits32(base_addr + RCC_CSR_OFFSET, RCC_CSR_RMVF);
    }
}

void rcc_enable_peripheral_clock(uintptr_t base_addr, uint32_t bus_reg_offset, uint32_t periph_mask)
{
    if (base_addr != 0) {
        mmio_set_bits32(base_addr + bus_reg_offset, periph_mask);
    }
}

void rcc_disable_peripheral_clock(uintptr_t base_addr, uint32_t bus_reg_offset, uint32_t periph_mask)
{
    if (base_addr != 0) {
        mmio_clear_bits32(base_addr + bus_reg_offset, periph_mask);
    }
}

