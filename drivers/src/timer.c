/**
 * @file timer.c
 * @brief System Timer, SysTick & Microsecond Precision Delay Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "timer.h"
#include "mmio.h"
#include "soc_regs.h"

static uintptr_t s_timer_base = 0;
static uint32_t s_timer_clock_hz = 25000000; /* Default 25 MHz */
static volatile uint64_t s_system_ticks = 0;

int timer_init(uintptr_t base_addr, uint32_t clock_hz)
{
    s_timer_base = base_addr;
    if (clock_hz > 0) {
        s_timer_clock_hz = clock_hz;
    }

#if defined(ARCH_RISCV)
    /* RISC-V CLINT Timer Setup: Base + 0x4000 = mtimecmp, Base + 0xBFF8 = mtime */
    /* Set mtimecmp high so timer doesn't trigger immediately */
    mmio_write32(SOC_CLINT_BASE + 0x4000, 0xFFFFFFFF);
    mmio_write32(SOC_CLINT_BASE + 0x4004, 0xFFFFFFFF);
#else
    /* ARM CMSDK / Standard Down-Counter Timer */
    if (base_addr != 0) {
        /* Stop timer */
        mmio_write32(base_addr + TIMER_CTRL_OFFSET, 0);
        /* Set default 1ms reload: (clock_hz / 1000) - 1 */
        uint32_t reload = (s_timer_clock_hz / 1000) - 1;
        mmio_write32(base_addr + TIMER_RELOAD_OFFSET, reload);
        mmio_write32(base_addr + TIMER_VALUE_OFFSET, reload);
        /* Clear any pending interrupt */
        mmio_write32(base_addr + TIMER_INTCLEAR_OFFSET, 1);
        /* Start timer with interrupts enabled */
        mmio_write32(base_addr + TIMER_CTRL_OFFSET, TIMER_CTRL_EN | TIMER_CTRL_INT_EN);
    }
#endif

    return 0;
}

void timer_set_periodic(uintptr_t base_addr, uint32_t period_us)
{
    if (base_addr == 0) return;
    uint32_t reload = (uint32_t)(((uint64_t)s_timer_clock_hz * period_us) / 1000000ULL);
    if (reload > 0) reload -= 1;

    mmio_write32(base_addr + TIMER_CTRL_OFFSET, 0);
    mmio_write32(base_addr + TIMER_RELOAD_OFFSET, reload);
    mmio_write32(base_addr + TIMER_VALUE_OFFSET, reload);
    mmio_write32(base_addr + TIMER_INTCLEAR_OFFSET, 1);
    mmio_write32(base_addr + TIMER_CTRL_OFFSET, TIMER_CTRL_EN | TIMER_CTRL_INT_EN);
}

void timer_start(uintptr_t base_addr)
{
    if (base_addr != 0) {
        mmio_set_bits32(base_addr + TIMER_CTRL_OFFSET, TIMER_CTRL_EN);
    }
}

void timer_stop(uintptr_t base_addr)
{
    if (base_addr != 0) {
        mmio_clear_bits32(base_addr + TIMER_CTRL_OFFSET, TIMER_CTRL_EN);
    }
}

void timer_clear_irq(uintptr_t base_addr)
{
    if (base_addr != 0) {
        mmio_write32(base_addr + TIMER_INTCLEAR_OFFSET, 1);
    }
}

uint32_t timer_get_ticks(uintptr_t base_addr)
{
    if (base_addr != 0) {
        return mmio_read32(base_addr + TIMER_VALUE_OFFSET);
    }
    return (uint32_t)s_system_ticks;
}

uint64_t timer_get_time_us(void)
{
#if defined(ARCH_RISCV)
    /* Read 64-bit mtime register from CLINT (0xBFF8) */
    uint32_t low, high, high2;
    do {
        high = mmio_read32(SOC_CLINT_BASE + 0xBFFC);
        low  = mmio_read32(SOC_CLINT_BASE + 0xBFF8);
        high2 = mmio_read32(SOC_CLINT_BASE + 0xBFFC);
    } while (high != high2);
    uint64_t mtime = (((uint64_t)high) << 32) | low;
    /* Virt CLINT frequency is typically 10 MHz */
    return mtime / 10ULL;
#else
    /* Synthesized timestamp using tick counter and elapsed reload ticks */
    if (s_timer_base != 0) {
        uint32_t reload = mmio_read32(s_timer_base + TIMER_RELOAD_OFFSET);
        uint32_t current = mmio_read32(s_timer_base + TIMER_VALUE_OFFSET);
        uint32_t elapsed_in_period = (reload > current) ? (reload - current) : 0;
        uint64_t us_in_period = ((uint64_t)elapsed_in_period * 1000000ULL) / s_timer_clock_hz;
        return (s_system_ticks * 1000ULL) + us_in_period;
    }
    return s_system_ticks * 1000ULL;
#endif
}

uint32_t timer_get_time_ms(void)
{
    return (uint32_t)(timer_get_time_us() / 1000ULL);
}

void delay_us(uint32_t us)
{
    uint64_t start = timer_get_time_us();
    if (s_timer_base != 0 || start != 0) {
        while ((timer_get_time_us() - start) < us) {
            /* Spin */
        }
    } else {
        /* Calibrated loop fallback if timer peripheral uninitialized */
        /* Assuming ~4 cycles per loop at 25MHz -> ~6 iterations per us */
        volatile uint32_t loops = us * 6;
        while (loops--) {
            __asm__ volatile ("" ::: "memory");
        }
    }
}

void delay_ms(uint32_t ms)
{
    while (ms--) {
        delay_us(1000);
    }
}

/* System tick interrupt handler called by assembly vector */
void SysTick_Handler(void)
{
    s_system_ticks++;
    if (s_timer_base != 0) {
        timer_clear_irq(s_timer_base);
    }
}
