/**
 * @file gpio.h
 * @brief General Purpose Input/Output (GPIO) & Pin Control Driver
 *
 * Provides bit-level pin manipulation, input reading, output driving,
 * pull-up/pull-down resistor configuration, and edge interrupt testing.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_ALT = 2,
    GPIO_MODE_ANALOG = 3
} gpio_mode_t;

typedef enum {
    GPIO_PULL_NONE = 0,
    GPIO_PULL_UP = 1,
    GPIO_PULL_DOWN = 2
} gpio_pull_t;

typedef enum {
    GPIO_SPEED_LOW = 0,
    GPIO_SPEED_MEDIUM = 1,
    GPIO_SPEED_HIGH = 2,
    GPIO_SPEED_VERY_HIGH = 3
} gpio_speed_t;

typedef struct {
    uint8_t pin;
    gpio_mode_t mode;
    gpio_pull_t pull;
    gpio_speed_t speed;
    uint8_t alt_function;
} gpio_pin_config_t;

/**
 * @brief Configure a specific GPIO pin on port.
 * @param port_base MMIO base address of GPIO port
 * @param config Pointer to pin configuration
 */
void gpio_config_pin(uintptr_t port_base, const gpio_pin_config_t *config);

/**
 * @brief Write digital state to pin (1 or 0).
 * @param port_base MMIO base address
 * @param pin Pin index (0..15 or 0..31)
 * @param state true = High, false = Low
 */
void gpio_write_pin(uintptr_t port_base, uint8_t pin, bool state);

/**
 * @brief Read digital state from pin.
 * @param port_base MMIO base address
 * @param pin Pin index
 * @return true if pin is High, false if Low
 */
bool gpio_read_pin(uintptr_t port_base, uint8_t pin);

/**
 * @brief Toggle digital state of pin.
 * @param port_base MMIO base address
 * @param pin Pin index
 */
void gpio_toggle_pin(uintptr_t port_base, uint8_t pin);

/**
 * @brief Write entire port output register.
 * @param port_base MMIO base address
 * @param val 32-bit port value
 */
void gpio_write_port(uintptr_t port_base, uint32_t val);

/**
 * @brief Read entire port input register.
 * @param port_base MMIO base address
 * @return 32-bit port state
 */
uint32_t gpio_read_port(uintptr_t port_base);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */

