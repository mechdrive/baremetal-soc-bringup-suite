/**
 * @file timer.h
 * @brief Core System Timer, SysTick & Microsecond Precision Delay Driver
 *
 * Implements high-resolution hardware timekeeping, monotonic timestamps,
 * periodic timer interrupt management, and calibrated microsecond/millisecond busywaits.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize system timer with base peripheral address and input clock frequency.
 * @param base_addr MMIO address of timer controller
 * @param clock_hz Peripheral bus clock in Hertz
 * @return 0 on success, negative on error
 */
int timer_init(uintptr_t base_addr, uint32_t clock_hz);

/**
 * @brief Configure periodic timer interrupt interval.
 * @param base_addr MMIO address
 * @param period_us Microseconds between periodic interrupts
 */
void timer_set_periodic(uintptr_t base_addr, uint32_t period_us);

/**
 * @brief Start timer counting.
 * @param base_addr MMIO address
 */
void timer_start(uintptr_t base_addr);

/**
 * @brief Stop timer counting.
 * @param base_addr MMIO address
 */
void timer_stop(uintptr_t base_addr);

/**
 * @brief Clear pending interrupt flag on timer.
 * @param base_addr MMIO address
 */
void timer_clear_irq(uintptr_t base_addr);

/**
 * @brief Read current timer tick count (down or up counter).
 * @param base_addr MMIO address
 * @return Raw counter value
 */
uint32_t timer_get_ticks(uintptr_t base_addr);

/**
 * @brief Get high-resolution monotonic microsecond timestamp since boot.
 * @return Microseconds elapsed
 */
uint64_t timer_get_time_us(void);

/**
 * @brief Get monotonic millisecond timestamp since boot.
 * @return Milliseconds elapsed
 */
uint32_t timer_get_time_ms(void);

/**
 * @brief Precise busy-wait delay in microseconds.
 * @param us Microseconds to delay
 */
void delay_us(uint32_t us);

/**
 * @brief Busy-wait delay in milliseconds.
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H */
