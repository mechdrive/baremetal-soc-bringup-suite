/**
 * @file i2c.c
 * @brief I2C Master Driver & Bus Lockup Recovery Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "i2c.h"
#include "gpio.h"
#include "timer.h"
#include "mmio.h"
#include "soc_regs.h"

#define I2C_TIMEOUT_CYCLES  50000U

int i2c_init(uintptr_t base_addr, uint32_t speed_hz, uint32_t pclk_hz)
{
    if (base_addr == 0 || speed_hz == 0 || pclk_hz == 0) return -1;

    /* 1. Reset I2C peripheral */
    mmio_set_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_SWRST);
    delay_us(10);
    mmio_clear_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_SWRST);

    /* 2. Configure input clock frequency in MHz into CR2 */
    uint32_t pclk_mhz = pclk_hz / 1000000UL;
    if (pclk_mhz < 2) pclk_mhz = 2;
    mmio_write32(base_addr + I2C_CR2_OFFSET, pclk_mhz & 0x3F);

    /* 3. Configure Clock Control Register (CCR) */
    /* Standard mode (100kHz): Thigh = Tlow = CCR * TPCLK1 -> CCR = FPCLK1 / (2 * Fscl) */
    uint32_t ccr = pclk_hz / (2 * speed_hz);
    if (ccr < 4) ccr = 4;
    mmio_write32(base_addr + I2C_CCR_OFFSET, ccr);

    /* 4. Configure Maximum Rise Time: TRISE = (Trise_max / TPCLK1) + 1 */
    /* For 100kHz standard mode, Trise_max is 1000ns */
    uint32_t trise = pclk_mhz + 1;
    mmio_write32(base_addr + I2C_TRISE_OFFSET, trise);

    /* 5. Enable I2C peripheral with ACK enabled */
    mmio_write32(base_addr + I2C_CR1_OFFSET, I2C_CR1_PE | I2C_CR1_ACK);

    return I2C_OK;
}

static bool i2c_wait_flag_set(uintptr_t base_addr, uint32_t offset, uint32_t mask)
{
    uint32_t timeout = I2C_TIMEOUT_CYCLES;
    while ((mmio_read32(base_addr + offset) & mask) == 0) {
        if (--timeout == 0) return false;
    }
    return true;
}

static bool i2c_generate_start(uintptr_t base_addr)
{
    mmio_set_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_START);
    return i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_SB);
}

static void i2c_generate_stop(uintptr_t base_addr)
{
    mmio_set_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_STOP);
}

bool i2c_probe_device(uintptr_t base_addr, uint8_t addr_7bit)
{
    if (base_addr == 0) return false;

    /* Generate START condition */
    if (!i2c_generate_start(base_addr)) {
        i2c_generate_stop(base_addr);
        return false;
    }

    /* Send target address with Write bit (0) */
    mmio_write8(base_addr + I2C_DR_OFFSET, (uint8_t)(addr_7bit << 1));

    /* Wait for ADDR flag or AF (Acknowledge Failure) */
    uint32_t timeout = I2C_TIMEOUT_CYCLES;
    while (timeout--) {
        uint32_t sr1 = mmio_read32(base_addr + I2C_SR1_OFFSET);
        if (sr1 & I2C_SR1_AF) {
            /* Device did not acknowledge address */
            mmio_clear_bits32(base_addr + I2C_SR1_OFFSET, I2C_SR1_AF);
            i2c_generate_stop(base_addr);
            return false;
        }
        if (sr1 & I2C_SR1_ADDR) {
            /* Address matched! Clear ADDR flag by reading SR1 followed by SR2 */
            (void)mmio_read32(base_addr + I2C_SR1_OFFSET);
            (void)mmio_read32(base_addr + I2C_SR2_OFFSET);
            i2c_generate_stop(base_addr);
            return true;
        }
    }

    i2c_generate_stop(base_addr);
    return false;
}

i2c_status_t i2c_write(uintptr_t base_addr, uint8_t addr_7bit, const uint8_t *data, size_t len)
{
    if (base_addr == 0 || (data == 0 && len > 0)) return I2C_ERR_BUSY;

    if (!i2c_generate_start(base_addr)) {
        return I2C_ERR_START_FAILED;
    }

    /* Send slave address + Write */
    mmio_write8(base_addr + I2C_DR_OFFSET, (uint8_t)(addr_7bit << 1));
    if (!i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_ADDR)) {
        i2c_generate_stop(base_addr);
        return I2C_ERR_ADDR_NACK;
    }
    (void)mmio_read32(base_addr + I2C_SR1_OFFSET);
    (void)mmio_read32(base_addr + I2C_SR2_OFFSET);

    /* Write data bytes */
    for (size_t i = 0; i < len; ++i) {
        if (!i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_TXE)) {
            i2c_generate_stop(base_addr);
            return I2C_ERR_TIMEOUT;
        }
        mmio_write8(base_addr + I2C_DR_OFFSET, data[i]);
    }

    /* Wait for byte transfer to finish (BTF) */
    if (!i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_BTF)) {
        i2c_generate_stop(base_addr);
        return I2C_ERR_TIMEOUT;
    }

    i2c_generate_stop(base_addr);
    return I2C_OK;
}

i2c_status_t i2c_read(uintptr_t base_addr, uint8_t addr_7bit, uint8_t *data, size_t len)
{
    if (base_addr == 0 || data == 0 || len == 0) return I2C_ERR_BUSY;

    if (!i2c_generate_start(base_addr)) {
        return I2C_ERR_START_FAILED;
    }

    /* Send slave address + Read (bit 0 = 1) */
    mmio_write8(base_addr + I2C_DR_OFFSET, (uint8_t)((addr_7bit << 1) | 0x01));
    if (!i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_ADDR)) {
        i2c_generate_stop(base_addr);
        return I2C_ERR_ADDR_NACK;
    }

    if (len == 1) {
        /* Single byte read: clear ACK bit before clearing ADDR */
        mmio_clear_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_ACK);
        (void)mmio_read32(base_addr + I2C_SR1_OFFSET);
        (void)mmio_read32(base_addr + I2C_SR2_OFFSET);
        i2c_generate_stop(base_addr);

        if (!i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_RXNE)) {
            return I2C_ERR_TIMEOUT;
        }
        data[0] = mmio_read8(base_addr + I2C_DR_OFFSET);
    } else {
        (void)mmio_read32(base_addr + I2C_SR1_OFFSET);
        (void)mmio_read32(base_addr + I2C_SR2_OFFSET);

        for (size_t i = 0; i < len; ++i) {
            if (i == len - 1) {
                /* NACK last byte */
                mmio_clear_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_ACK);
                i2c_generate_stop(base_addr);
            }
            if (!i2c_wait_flag_set(base_addr, I2C_SR1_OFFSET, I2C_SR1_RXNE)) {
                return I2C_ERR_TIMEOUT;
            }
            data[i] = mmio_read8(base_addr + I2C_DR_OFFSET);
        }
    }

    /* Re-enable ACK for subsequent transfers */
    mmio_set_bits32(base_addr + I2C_CR1_OFFSET, I2C_CR1_ACK);
    return I2C_OK;
}

i2c_status_t i2c_read_reg(uintptr_t base_addr, uint8_t addr_7bit, uint8_t reg_addr, uint8_t *data, size_t len)
{
    /* 1. Write register address */
    i2c_status_t status = i2c_write(base_addr, addr_7bit, &reg_addr, 1);
    if (status != I2C_OK) return status;

    /* 2. Read contents */
    return i2c_read(base_addr, addr_7bit, data, len);
}

void i2c_bus_recovery_sequence(uintptr_t scl_gpio_base, uint8_t scl_pin,
                               uintptr_t sda_gpio_base, uint8_t sda_pin)
{
    /* Industry standard I2C recovery protocol:
     * When a slave device locks up holding SDA LOW in the middle of a read,
     * the master reconfigures SCL/SDA as GPIO outputs and toggles SCL 9 times.
     * This clock pulses the slave state machine until it releases SDA HIGH.
     * Finally, generate a STOP condition.
     */
    gpio_pin_config_t cfg_out = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = GPIO_PULL_UP,
        .speed = GPIO_SPEED_HIGH,
        .alt_function = 0
    };

    cfg_out.pin = scl_pin;
    gpio_config_pin(scl_gpio_base, &cfg_out);
    cfg_out.pin = sda_pin;
    gpio_config_pin(sda_gpio_base, &cfg_out);

    gpio_write_pin(sda_gpio_base, sda_pin, true);
    gpio_write_pin(scl_gpio_base, scl_pin, true);
    delay_us(10);

    /* 9 clock pulses on SCL */
    for (int i = 0; i < 9; ++i) {
        gpio_write_pin(scl_gpio_base, scl_pin, false);
        delay_us(5);
        gpio_write_pin(scl_gpio_base, scl_pin, true);
        delay_us(5);
    }

    /* Generate STOP condition manually: SDA Low while SCL High, then SDA High */
    gpio_write_pin(sda_gpio_base, sda_pin, false);
    delay_us(5);
    gpio_write_pin(scl_gpio_base, scl_pin, true);
    delay_us(5);
    gpio_write_pin(sda_gpio_base, sda_pin, true);
    delay_us(10);
}

