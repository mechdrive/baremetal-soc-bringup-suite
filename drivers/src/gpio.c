/**
 * @file gpio.c
 * @brief General Purpose Input/Output (GPIO) Driver Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "gpio.h"
#include "mmio.h"
#include "soc_regs.h"

void gpio_config_pin(uintptr_t port_base, const gpio_pin_config_t *config)
{
    if (port_base == 0 || config == 0 || config->pin >= 16) return;

    uint8_t pin = config->pin;
    uint32_t shift2 = pin * 2;

    /* 1. Configure Pin Mode (2 bits per pin) */
    mmio_modify32(port_base + GPIO_MODER_OFFSET, (3U << shift2), ((uint32_t)config->mode << shift2));

    /* 2. Configure Pull-up / Pull-down (2 bits per pin) */
    mmio_modify32(port_base + GPIO_PUPDR_OFFSET, (3U << shift2), ((uint32_t)config->pull << shift2));

    /* 3. Configure Output Speed (2 bits per pin) */
    mmio_modify32(port_base + GPIO_OSPEEDR_OFFSET, (3U << shift2), ((uint32_t)config->speed << shift2));

    /* 4. Configure Alternate Function (4 bits per pin) */
    if (config->mode == GPIO_MODE_ALT) {
        if (pin < 8) {
            uint32_t shift4 = pin * 4;
            mmio_modify32(port_base + GPIO_AFRL_OFFSET, (0xFU << shift4), ((uint32_t)config->alt_function << shift4));
        } else {
            uint32_t shift4 = (pin - 8) * 4;
            mmio_modify32(port_base + GPIO_AFRH_OFFSET, (0xFU << shift4), ((uint32_t)config->alt_function << shift4));
        }
    }
}

void gpio_write_pin(uintptr_t port_base, uint8_t pin, bool state)
{
    if (port_base == 0 || pin >= 16) return;

    /* Using Bit Set/Reset Register (BSRR) for atomic write */
    if (state) {
        mmio_write32(port_base + GPIO_BSRR_OFFSET, (1U << pin));
    } else {
        mmio_write32(port_base + GPIO_BSRR_OFFSET, (1U << (pin + 16)));
    }
}

bool gpio_read_pin(uintptr_t port_base, uint8_t pin)
{
    if (port_base == 0 || pin >= 16) return false;
    uint32_t idr = mmio_read32(port_base + GPIO_IDR_OFFSET);
    return (idr & (1U << pin)) != 0;
}

void gpio_toggle_pin(uintptr_t port_base, uint8_t pin)
{
    if (port_base == 0 || pin >= 16) return;
    mmio_toggle_bits32(port_base + GPIO_ODR_OFFSET, (1U << pin));
}

void gpio_write_port(uintptr_t port_base, uint32_t val)
{
    if (port_base != 0) {
        mmio_write32(port_base + GPIO_ODR_OFFSET, val);
    }
}

uint32_t gpio_read_port(uintptr_t port_base)
{
    if (port_base != 0) {
        return mmio_read32(port_base + GPIO_IDR_OFFSET);
    }
    return 0;
}
