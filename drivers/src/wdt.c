/**
 * @file wdt.c
 * @brief Watchdog Timer Driver Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "wdt.h"
#include "mmio.h"
#include "soc_regs.h"

int wdt_init(uintptr_t base_addr, const wdt_config_t *config)
{
    if (base_addr == 0 || config == 0) return -1;

    /* 1. Unlock Watchdog registers */
    mmio_write32(base_addr + WDT_LOCK_OFFSET, WDT_UNLOCK_KEY);

    /* 2. Configure countdown reload value: assuming 25MHz clock */
    /* reload = (25000000 / 1000) * timeout_ms */
    uint32_t load = 25000UL * config->timeout_ms;
    if (load == 0) load = 2500000UL; /* Default 100ms */
    mmio_write32(base_addr + WDT_LOAD_OFFSET, load);

    /* 3. Configure Control Register */
    uint32_t ctrl = 0;
    if (config->enable_irq) {
        ctrl |= WDT_CTRL_INTEN;
    }
    if (config->enable_reset) {
        ctrl |= WDT_CTRL_RESEN;
    }
    mmio_write32(base_addr + WDT_CTRL_OFFSET, ctrl);

    /* 4. Lock Watchdog registers to protect from runaway code */
    mmio_write32(base_addr + WDT_LOCK_OFFSET, 0x00000000);

    return 0;
}

void wdt_feed(uintptr_t base_addr)
{
    if (base_addr == 0) return;
    /* Unlock, clear counter/interrupt, relock */
    mmio_write32(base_addr + WDT_LOCK_OFFSET, WDT_UNLOCK_KEY);
    mmio_write32(base_addr + WDT_INTCLR_OFFSET, 0x01);
    mmio_write32(base_addr + WDT_LOCK_OFFSET, 0x00000000);
}

uint32_t wdt_get_current_count(uintptr_t base_addr)
{
    if (base_addr == 0) return 0;
    return mmio_read32(base_addr + WDT_VALUE_OFFSET);
}

void wdt_force_reset_test(uintptr_t base_addr)
{
    if (base_addr == 0) return;
    /* Set load to 1 and enable reset to trip watchdog immediately */
    mmio_write32(base_addr + WDT_LOCK_OFFSET, WDT_UNLOCK_KEY);
    mmio_write32(base_addr + WDT_LOAD_OFFSET, 0x01);
    mmio_write32(base_addr + WDT_CTRL_OFFSET, WDT_CTRL_INTEN | WDT_CTRL_RESEN);
    /* Busy wait for reset */
    while (1) {
        __asm__ volatile ("" ::: "memory");
    }
}

