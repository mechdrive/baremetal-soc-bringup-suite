/**
 * @file spi.c
 * @brief Serial Peripheral Interface (SPI) Master Driver Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "spi.h"
#include "mmio.h"
#include "soc_regs.h"

int spi_init(uintptr_t base_addr, const spi_config_t *config)
{
    if (base_addr == 0 || config == 0) return -1;

    /* 1. Disable SPI while configuring */
    mmio_write32(base_addr + SPI_CR1_OFFSET, 0);

    /* 2. Configure CR1: Master mode, Baudrate, CPOL, CPHA, Bit order */
    uint32_t cr1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM;
    cr1 |= ((uint32_t)config->prescaler << 3);

    if (config->mode == SPI_MODE_1 || config->mode == SPI_MODE_3) {
        cr1 |= SPI_CR1_CPHA;
    }
    if (config->mode == SPI_MODE_2 || config->mode == SPI_MODE_3) {
        cr1 |= SPI_CR1_CPOL;
    }
    if (!config->msb_first) {
        cr1 |= SPI_CR1_LSBFIRST;
    }

    mmio_write32(base_addr + SPI_CR1_OFFSET, cr1);

    /* 3. Configure CR2: 8-bit data size by default */
    mmio_write32(base_addr + SPI_CR2_OFFSET, 0x0700); /* 8-bit FIFO threshold */

    /* 4. Enable SPI */
    mmio_set_bits32(base_addr + SPI_CR1_OFFSET, SPI_CR1_SPE);

    if (config->enable_loopback) {
        spi_set_loopback(base_addr, true);
    }

    return 0;
}

uint8_t spi_transfer_byte(uintptr_t base_addr, uint8_t tx_byte)
{
    if (base_addr == 0) return 0xFF;

    /* Wait until TX buffer is empty: SR bit 1 (TXE) */
    while ((mmio_read32(base_addr + SPI_SR_OFFSET) & SPI_SR_TXE) == 0) {
        /* Busy wait */
    }

    /* Write data byte */
    mmio_write8(base_addr + SPI_DR_OFFSET, tx_byte);

    /* Wait until RX buffer is not empty: SR bit 0 (RXNE) */
    while ((mmio_read32(base_addr + SPI_SR_OFFSET) & SPI_SR_RXNE) == 0) {
        /* Busy wait */
    }

    /* Read and return received byte */
    return mmio_read8(base_addr + SPI_DR_OFFSET);
}

size_t spi_transfer(uintptr_t base_addr, const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
    if (base_addr == 0 || len == 0) return 0;

    for (size_t i = 0; i < len; ++i) {
        uint8_t tx_byte = tx_buf ? tx_buf[i] : 0xFF;
        uint8_t rx_byte = spi_transfer_byte(base_addr, tx_byte);
        if (rx_buf) {
            rx_buf[i] = rx_byte;
        }
    }
    return len;
}

void spi_set_loopback(uintptr_t base_addr, bool enable)
{
    if (base_addr == 0) return;
    if (enable) {
        /* Set bidirectional mode & output enable for internal loopback */
        mmio_set_bits32(base_addr + SPI_CR1_OFFSET, SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);
    } else {
        mmio_clear_bits32(base_addr + SPI_CR1_OFFSET, SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);
    }
}

