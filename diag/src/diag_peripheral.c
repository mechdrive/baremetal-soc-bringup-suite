/**
 * @file diag_peripheral.c
 * @brief Peripheral Diagnostic Test Implementations
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "diag_peripheral.h"
#include "bringup_framework.h"
#include "soc_regs.h"
#include "rcc.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"
#include "wdt.h"
#include "dma.h"
#include <string.h>

/* Test 1: Clock Tree & PLL Lock */
test_result_t test_periph_clock_tree(void)
{
    clock_tree_config_t cfg = {
        .target_source = CLK_SRC_PLL,
        .pll_multiplier = 8,
        .pll_prediv = 2,
        .ahb_prescaler = 0,
        .apb1_prescaler = 2,
        .apb2_prescaler = 1
    };

    int res = rcc_init_clocks(SOC_RCC_BASE, &cfg, 100000);
    ASSERT_EQ(0, res, "Clock initialization and PLL lock failed");

    reset_reason_t reason = rcc_get_reset_reason(SOC_RCC_BASE);
    diag_log(LOG_LEVEL_INFO, "Reset cause identified: %s", rcc_reset_reason_to_str(reason));

    return TEST_RESULT_PASS;
}

/* Test 2: UART Loopback */
test_result_t test_periph_uart_loopback(void)
{
    const char test_seq[] = "BRINGUP_UART_0x55AA_TEST";
    size_t len = strlen(test_seq);

    /* Enable loopback on UART0 */
    uart_set_loopback(SOC_UART0_BASE, true);

    for (size_t i = 0; i < len; ++i) {
        uart_putc(SOC_UART0_BASE, test_seq[i]);
        /* Read back */
        char echo = uart_getc(SOC_UART0_BASE);
        if (echo != test_seq[i]) {
            uart_set_loopback(SOC_UART0_BASE, false);
            ASSERT_EQ((uint32_t)test_seq[i], (uint32_t)echo, "UART loopback byte mismatch");
        }
    }

    uart_set_loopback(SOC_UART0_BASE, false);
    return TEST_RESULT_PASS;
}

/* Test 3: System Timer Precision & Busywait Delay */
test_result_t test_periph_timer_calibration(void)
{
    uint64_t t1 = timer_get_time_us();
    delay_us(1000); /* 1000us = 1ms */
    uint64_t t2 = timer_get_time_us();

    uint64_t elapsed = t2 - t1;
    /* Verify elapsed time is within expected tolerance (allowing +-15% for simulator overhead) */
    if (elapsed < 850 || elapsed > 1500) {
        ASSERT_EQ(1000, (uint32_t)elapsed, "Timer microsecond calibration out of tolerance");
    }

    return TEST_RESULT_PASS;
}

/* Test 4: GPIO Port & Pin Control */
test_result_t test_periph_gpio_toggle(void)
{
    gpio_pin_config_t cfg = {
        .pin = 5,
        .mode = GPIO_MODE_OUTPUT,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_HIGH,
        .alt_function = 0
    };

    gpio_config_pin(SOC_GPIO0_BASE, &cfg);

    /* Set pin High */
    gpio_write_pin(SOC_GPIO0_BASE, 5, true);
    ASSERT_TRUE(gpio_read_pin(SOC_GPIO0_BASE, 5), "GPIO pin 5 failed to drive High");

    /* Clear pin Low */
    gpio_write_pin(SOC_GPIO0_BASE, 5, false);
    ASSERT_FALSE(gpio_read_pin(SOC_GPIO0_BASE, 5), "GPIO pin 5 failed to drive Low");

    /* Toggle pin */
    gpio_toggle_pin(SOC_GPIO0_BASE, 5);
    ASSERT_TRUE(gpio_read_pin(SOC_GPIO0_BASE, 5), "GPIO pin 5 failed to toggle");

    return TEST_RESULT_PASS;
}

/* Test 5: SPI Master Loopback */
test_result_t test_periph_spi_loopback(void)
{
    spi_config_t cfg = {
        .mode = SPI_MODE_0,
        .prescaler = SPI_BAUD_DIV4,
        .msb_first = true,
        .enable_loopback = true
    };

    int init_res = spi_init(SOC_SPI0_BASE, &cfg);
    ASSERT_EQ(0, init_res, "SPI master controller initialization failed");

    uint8_t tx_data[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0x55 };
    uint8_t rx_data[sizeof(tx_data)] = { 0 };

    size_t xfer = spi_transfer(SOC_SPI0_BASE, tx_data, rx_data, sizeof(tx_data));
    ASSERT_EQ(sizeof(tx_data), xfer, "SPI transfer length mismatch");

    for (size_t i = 0; i < sizeof(tx_data); ++i) {
        if (rx_data[i] != tx_data[i]) {
            ASSERT_EQ((uint32_t)tx_data[i], (uint32_t)rx_data[i], "SPI loopback data corruption");
        }
    }

    return TEST_RESULT_PASS;
}

/* Test 6: I2C Master Probe & Recovery Sequence */
test_result_t test_periph_i2c_bus(void)
{
    /* Initialize I2C controller at 100 kHz */
    int init_res = i2c_init(SOC_I2C0_BASE, 100000, 25000000);
    ASSERT_EQ(I2C_OK, init_res, "I2C master initialization failed");

    /* Test 9-clock bus unwedge recovery routine */
    i2c_bus_recovery_sequence(SOC_GPIO0_BASE, 8, SOC_GPIO0_BASE, 9);

    /* Probe simulated device address 0x50 (EEPROM) */
    bool ack = i2c_probe_device(SOC_I2C0_BASE, 0x50);
    ASSERT_TRUE(ack, "I2C device at 0x50 failed to ACK probe");

    /* Write 4 bytes to register 0x00 */
    uint8_t write_payload[] = { 0x00, 0x11, 0x22, 0x33 };
    i2c_status_t wr_status = i2c_write(SOC_I2C0_BASE, 0x50, write_payload, sizeof(write_payload));
    ASSERT_EQ(I2C_OK, wr_status, "I2C block write failed");

    /* Read back 3 bytes from register 0x00 */
    uint8_t read_buf[3] = { 0 };
    i2c_status_t rd_status = i2c_read_reg(SOC_I2C0_BASE, 0x50, 0x00, read_buf, sizeof(read_buf));
    ASSERT_EQ(I2C_OK, rd_status, "I2C register read failed");

    ASSERT_EQ(0x11, read_buf[0], "I2C read payload byte 0 mismatch");
    ASSERT_EQ(0x22, read_buf[1], "I2C read payload byte 1 mismatch");
    ASSERT_EQ(0x33, read_buf[2], "I2C read payload byte 2 mismatch");

    return TEST_RESULT_PASS;
}

/* Test 7: Watchdog Supervision & Refresh */
test_result_t test_periph_wdt_supervision(void)
{
    wdt_config_t cfg = {
        .timeout_ms = 50,
        .enable_reset = false,
        .enable_irq = true
    };

    int init_res = wdt_init(SOC_WDT_BASE, &cfg);
    ASSERT_EQ(0, init_res, "Watchdog initialization failed");

    /* Check counter decrement */
    uint32_t c1 = wdt_get_current_count(SOC_WDT_BASE);
    delay_us(100);
    uint32_t c2 = wdt_get_current_count(SOC_WDT_BASE);
    ASSERT_TRUE(c2 < c1, "Watchdog counter failed to decrement");

    /* Feed watchdog */
    wdt_feed(SOC_WDT_BASE);
    uint32_t c3 = wdt_get_current_count(SOC_WDT_BASE);
    ASSERT_TRUE(c3 >= c2, "Watchdog feed failed to reload counter");

    return TEST_RESULT_PASS;
}

/* Test 8: DMA Block Memory Copy */
test_result_t test_periph_dma_transfer(void)
{
    int init_res = dma_init(SOC_DMA0_BASE);
    ASSERT_EQ(0, init_res, "DMA controller initialization failed");

    uint32_t src_block[64];
    uint32_t dst_block[64];

    for (size_t i = 0; i < 64; ++i) {
        src_block[i] = (uint32_t)(0xBEEF0000UL | (i * 0x101UL));
        dst_block[i] = 0x00000000UL;
    }

    bool success = dma_memcpy_sync(SOC_DMA0_BASE, 0, dst_block, src_block, sizeof(src_block), 100000);
    ASSERT_TRUE(success, "DMA synchronous memory block copy timed out or failed");

    for (size_t i = 0; i < 64; ++i) {
        if (dst_block[i] != src_block[i]) {
            ASSERT_EQ(src_block[i], dst_block[i], "DMA transferred data mismatch");
        }
    }

    return TEST_RESULT_PASS;
}

void diag_peripheral_register_all(void)
{
    test_case_t tc_clk = {
        .test_id = 20,
        .name = "PERIPH_CLOCK_PLL",
        .description = "Oscillator stability, PLL lock & reset reason decode",
        .phase = PHASE_CLOCK_POWER,
        .test_fn = test_periph_clock_tree
    };
    bringup_register_test(&tc_clk);

    test_case_t tc_uart = {
        .test_id = 21,
        .name = "PERIPH_UART_LOOPBACK",
        .description = "Serial controller FIFO loopback transmission",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_uart_loopback
    };
    bringup_register_test(&tc_uart);

    test_case_t tc_tmr = {
        .test_id = 22,
        .name = "PERIPH_TIMER_CALIB",
        .description = "Microsecond delay & counter drift verification",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_timer_calibration
    };
    bringup_register_test(&tc_tmr);

    test_case_t tc_gpio = {
        .test_id = 23,
        .name = "PERIPH_GPIO_TOGGLE",
        .description = "Pin output drive, atomic bit set/reset & readback",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_gpio_toggle
    };
    bringup_register_test(&tc_gpio);

    test_case_t tc_spi = {
        .test_id = 24,
        .name = "PERIPH_SPI_LOOPBACK",
        .description = "Full-duplex SPI master bus transaction integrity",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_spi_loopback
    };
    bringup_register_test(&tc_spi);

    test_case_t tc_i2c = {
        .test_id = 25,
        .name = "PERIPH_I2C_BUS_UNWEDGE",
        .description = "I2C probe, ACK verification & 9-clock bus recovery",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_i2c_bus
    };
    bringup_register_test(&tc_i2c);

    test_case_t tc_wdt = {
        .test_id = 26,
        .name = "PERIPH_WDT_SUPERVISION",
        .description = "Watchdog reload countdown & pet verification",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_wdt_supervision
    };
    bringup_register_test(&tc_wdt);

    test_case_t tc_dma = {
        .test_id = 27,
        .name = "PERIPH_DMA_MEMCPY",
        .description = "32-bit DMA block transfer integrity & throughput",
        .phase = PHASE_PERIPHERALS,
        .test_fn = test_periph_dma_transfer
    };
    bringup_register_test(&tc_dma);
}
