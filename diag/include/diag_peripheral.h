/**
 * @file diag_peripheral.h
 * @brief Peripheral Diagnostic Test Declarations
 *
 * Covers Clock Tree, UART, Timer, GPIO, SPI, I2C, WDT, and DMA.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef DIAG_PERIPHERAL_H
#define DIAG_PERIPHERAL_H

#include "diag_common.h"

#ifdef __cplusplus
extern "C" {
#endif

test_result_t test_periph_clock_tree(void);
test_result_t test_periph_uart_loopback(void);
test_result_t test_periph_timer_calibration(void);
test_result_t test_periph_gpio_toggle(void);
test_result_t test_periph_spi_loopback(void);
test_result_t test_periph_i2c_bus(void);
test_result_t test_periph_wdt_supervision(void);
test_result_t test_periph_dma_transfer(void);

void diag_peripheral_register_all(void);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_PERIPHERAL_H */

