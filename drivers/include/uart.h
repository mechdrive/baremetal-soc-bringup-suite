/**
 * @file uart.h
 * @brief High-Speed Universal Asynchronous Receiver Transmitter (UART) Driver
 *
 * Provides register-level bare-metal serial communications, FIFO buffering,
 * hardware loopback diagnostics, and non-blocking I/O.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} uart_parity_t;

typedef enum {
    UART_STOP_1 = 0,
    UART_STOP_2
} uart_stop_bits_t;

typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    bool enable_fifo;
    bool loopback_mode;
} uart_config_t;

/**
 * @brief Initialize UART controller with specific baud rate and line parameters.
 * @param base_addr Base MMIO address of UART peripheral
 * @param config Pointer to UART configuration struct
 * @return 0 on success, negative error code on invalid parameters
 */
int uart_init(uintptr_t base_addr, const uart_config_t *config);

/**
 * @brief Transmit a single character over UART (blocking).
 * @param base_addr Base MMIO address
 * @param c Character byte to send
 */
void uart_putc(uintptr_t base_addr, char c);

/**
 * @brief Receive a single character over UART (blocking).
 * @param base_addr Base MMIO address
 * @return Character received
 */
char uart_getc(uintptr_t base_addr);

/**
 * @brief Check if receive FIFO contains available data.
 * @param base_addr Base MMIO address
 * @return true if data is ready to read, false otherwise
 */
bool uart_has_rx(uintptr_t base_addr);

/**
 * @brief Check if transmit FIFO can accept data.
 * @param base_addr Base MMIO address
 * @return true if space available, false if full
 */
bool uart_can_tx(uintptr_t base_addr);

/**
 * @brief Transmit a null-terminated string over UART.
 * @param base_addr Base MMIO address
 * @param str String pointer
 */
void uart_puts(uintptr_t base_addr, const char *str);

/**
 * @brief Write raw buffer to UART.
 * @param base_addr Base MMIO address
 * @param buf Data buffer
 * @param len Number of bytes
 */
void uart_write(uintptr_t base_addr, const uint8_t *buf, size_t len);

/**
 * @brief Read bytes into buffer until length satisfied or timeout.
 * @param base_addr Base MMIO address
 * @param buf Destination buffer
 * @param max_len Maximum bytes to read
 * @param timeout_loops Loop count timeout per byte
 * @return Number of bytes actually read
 */
size_t uart_read(uintptr_t base_addr, uint8_t *buf, size_t max_len, uint32_t timeout_loops);

/**
 * @brief Configure internal loopback mode for diagnostic self-tests.
 * @param base_addr Base MMIO address
 * @param enable true to route TX directly into RX internally, false for normal
 */
void uart_set_loopback(uintptr_t base_addr, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
