/**
 * @file i2c.h
 * @brief Inter-Integrated Circuit (I2C) Master Bus Driver & Recovery Engine
 *
 * Provides master transmit/receive, device probing, ACK/NACK validation,
 * timeout supervision, and a hardware 9-clock bus un-wedging / lockup recovery sequence.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    I2C_SPEED_STANDARD = 100000, /* 100 kHz */
    I2C_SPEED_FAST     = 400000  /* 400 kHz */
} i2c_speed_t;

typedef enum {
    I2C_OK = 0,
    I2C_ERR_BUSY = -1,
    I2C_ERR_START_FAILED = -2,
    I2C_ERR_ADDR_NACK = -3,
    I2C_ERR_DATA_NACK = -4,
    I2C_ERR_TIMEOUT = -5,
    I2C_ERR_ARBITRATION_LOST = -6
} i2c_status_t;

/**
 * @brief Initialize I2C controller with bus speed and peripheral clock rate.
 * @param base_addr MMIO base address
 * @param speed_hz Target bus frequency (e.g. 100000 or 400000)
 * @param pclk_hz Peripheral clock feeding I2C controller in Hz
 * @return I2C_OK on success, negative error code otherwise
 */
int i2c_init(uintptr_t base_addr, uint32_t speed_hz, uint32_t pclk_hz);

/**
 * @brief Probe I2C bus for device presence at 7-bit address.
 * @param base_addr MMIO base address
 * @param addr_7bit 7-bit target slave address
 * @return true if device responded with ACK, false on NACK/timeout
 */
bool i2c_probe_device(uintptr_t base_addr, uint8_t addr_7bit);

/**
 * @brief Master write data buffer to slave device.
 * @param base_addr MMIO base address
 * @param addr_7bit 7-bit target slave address
 * @param data Buffer to transmit
 * @param len Length in bytes
 * @return I2C_OK or negative error status
 */
i2c_status_t i2c_write(uintptr_t base_addr, uint8_t addr_7bit, const uint8_t *data, size_t len);

/**
 * @brief Master read data buffer from slave device.
 * @param base_addr MMIO base address
 * @param addr_7bit 7-bit target slave address
 * @param data Destination buffer
 * @param len Length in bytes
 * @return I2C_OK or negative error status
 */
i2c_status_t i2c_read(uintptr_t base_addr, uint8_t addr_7bit, uint8_t *data, size_t len);

/**
 * @brief Perform standard register read (Write register address, Repeated-Start, Read data).
 * @param base_addr MMIO base address
 * @param addr_7bit 7-bit target slave address
 * @param reg_addr Target register on slave
 * @param data Destination buffer
 * @param len Length in bytes
 * @return I2C_OK or negative error status
 */
i2c_status_t i2c_read_reg(uintptr_t base_addr, uint8_t addr_7bit, uint8_t reg_addr, uint8_t *data, size_t len);

/**
 * @brief Execute 9-clock pulse sequence on SCL to clear stuck SDA slave lines.
 * @param scl_gpio_base GPIO port base for SCL
 * @param scl_pin SCL pin index
 * @param sda_gpio_base GPIO port base for SDA
 * @param sda_pin SDA pin index
 */
void i2c_bus_recovery_sequence(uintptr_t scl_gpio_base, uint8_t scl_pin,
                               uintptr_t sda_gpio_base, uint8_t sda_pin);

#ifdef __cplusplus
}
#endif

#endif /* I2C_H */

