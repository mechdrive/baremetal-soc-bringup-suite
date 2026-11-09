/**
 * @file spi.h
 * @brief Serial Peripheral Interface (SPI) Master Controller Driver
 *
 * Implements full-duplex synchronous bus communication, software chip select,
 * configurable CPOL/CPHA clock modes, baudrate prescalers, and loopback test mode.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SPI_MODE_0 = 0, /* CPOL=0, CPHA=0 */
    SPI_MODE_1 = 1, /* CPOL=0, CPHA=1 */
    SPI_MODE_2 = 2, /* CPOL=1, CPHA=0 */
    SPI_MODE_3 = 3  /* CPOL=1, CPHA=1 */
} spi_mode_t;

typedef enum {
    SPI_BAUD_DIV2 = 0,
    SPI_BAUD_DIV4 = 1,
    SPI_BAUD_DIV8 = 2,
    SPI_BAUD_DIV16 = 3,
    SPI_BAUD_DIV32 = 4,
    SPI_BAUD_DIV64 = 5,
    SPI_BAUD_DIV128 = 6,
    SPI_BAUD_DIV256 = 7
} spi_baud_prescaler_t;

typedef struct {
    spi_mode_t mode;
    spi_baud_prescaler_t prescaler;
    bool msb_first;
    bool enable_loopback;
} spi_config_t;

/**
 * @brief Initialize SPI controller with specified mode and clock division.
 * @param base_addr MMIO base address
 * @param config Pointer to SPI configuration
 * @return 0 on success, negative error code otherwise
 */
int spi_init(uintptr_t base_addr, const spi_config_t *config);

/**
 * @brief Perform full-duplex transfer of single 8-bit byte.
 * @param base_addr MMIO base address
 * @param tx_byte Byte to transmit
 * @return Byte simultaneously received
 */
uint8_t spi_transfer_byte(uintptr_t base_addr, uint8_t tx_byte);

/**
 * @brief Transfer buffer of data over SPI bus.
 * @param base_addr MMIO base address
 * @param tx_buf Transmit buffer (can be NULL if RX only)
 * @param rx_buf Receive buffer (can be NULL if TX only)
 * @param len Transfer length in bytes
 * @return Number of bytes exchanged
 */
size_t spi_transfer(uintptr_t base_addr, const uint8_t *tx_buf, uint8_t *rx_buf, size_t len);

/**
 * @brief Enable or disable hardware loopback mode for silicon self-test.
 * @param base_addr MMIO base address
 * @param enable true to enable loopback, false for external pins
 */
void spi_set_loopback(uintptr_t base_addr, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */

