/**
 * @file bringup_cli.c
 * @brief Interactive Silicon Bringup UART Monitor / Shell Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "bringup_cli.h"
#include "bringup_framework.h"
#include "diag_memory.h"
#include "diag_peripheral.h"
#include "diag_interrupt.h"
#include "diag_stress.h"
#include "uart.h"
#include "timer.h"
#include "rcc.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"
#include "wdt.h"
#include "dma.h"
#include "mmio.h"
#include "soc_regs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CLI_LINE_MAX 128
#define CLI_MAX_ARGS 8

static char s_line_buf[CLI_LINE_MAX];
static size_t s_line_idx = 0;

static uint32_t parse_hex_or_dec(const char *str)
{
    if (!str) return 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        return (uint32_t)strtoul(str, NULL, 16);
    }
    return (uint32_t)strtoul(str, NULL, 10);
}

static void cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    diag_log_raw("\r\n================= SILICON BRINGUP MONITOR COMMANDS =================\r\n");
    diag_log_raw("  help                                  : Display command summary\r\n");
    diag_log_raw("  version                               : Show hardware, architecture & build info\r\n");
    diag_log_raw("  status                                : Show SoC clock, uptime & reset reason\r\n");
    diag_log_raw("  peek <addr>                           : Read 32-bit MMIO register (hex)\r\n");
    diag_log_raw("  poke <addr> <val>                     : Write 32-bit MMIO register (hex)\r\n");
    diag_log_raw("  dump <addr> <len_bytes>               : Memory hex/ASCII dump\r\n");
    diag_log_raw("  fill <addr> <words> <val>             : Fill memory range with 32-bit word\r\n");
    diag_log_raw("  mtest <addr> <words> [all|march|lfsr] : Run memory algorithms on block\r\n");
    diag_log_raw("  periph [list|test]                    : Scan or test SoC peripherals\r\n");
    diag_log_raw("  clocks                                : Query PLL & clock tree status\r\n");
    diag_log_raw("  gpio <read|write|toggle> <pin> [val]  : Inspect or drive GPIO pin\r\n");
    diag_log_raw("  spi send <hex_byte>                   : Transfer byte via SPI master\r\n");
    diag_log_raw("  i2c probe <7bit_hex>                  : Probe I2C slave address\r\n");
    diag_log_raw("  wdt [feed|trip]                       : Refresh or force watchdog timeout\r\n");
    diag_log_raw("  dma test                              : Run DMA block transfer test\r\n");
    diag_log_raw("  diag [all|phase <1-7>|test <id>]      : Run automated silicon test suites\r\n");
    diag_log_raw("  reboot                                : Perform software SoC reset\r\n");
    diag_log_raw("====================================================================\r\n\r\n");
}

static void cmd_version(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[128];
#if defined(ARCH_RISCV)
    const char *arch_str = "RISC-V (RV32IM / Virt)";
#elif defined(ARCH_ARM)
    const char *arch_str = "ARM Cortex-M (ARMv7-M / MPS2)";
#else
    const char *arch_str = "Host Simulation Engine (x86_64 Virtual SoC)";
#endif

    diag_log_raw("\r\n+--------------------------------------------------------------+\r\n");
    diag_log_raw("| Bare-Metal Silicon Bringup & Peripheral Diagnostic Framework |\r\n");
    diag_log_raw("+--------------------------------------------------------------+\r\n");
    snprintf(buf, sizeof(buf), "  Architecture : %s\r\n", arch_str);
    diag_log_raw(buf);
    diag_log_raw("  Silicon Rev  : A0 (Tapeout Qualification)\r\n");
    diag_log_raw("  Author       : Poonam Singh\r\n");
    diag_log_raw("  Toolchain    : Bare-Metal GCC / Clang (No-OS Runtime)\r\n");
    diag_log_raw("+--------------------------------------------------------------+\r\n\r\n");
}

static void cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[128];
    uint32_t ms = timer_get_time_ms();
    reset_reason_t reason = rcc_get_reset_reason(SOC_RCC_BASE);

    diag_log_raw("\r\n--- SYSTEM STATUS ---\r\n");
    snprintf(buf, sizeof(buf), "  Uptime        : %lu ms (%lu seconds)\r\n",
             (unsigned long)ms, (unsigned long)(ms / 1000));
    diag_log_raw(buf);
    snprintf(buf, sizeof(buf), "  Reset Reason  : %s\r\n", rcc_reset_reason_to_str(reason));
    diag_log_raw(buf);
    snprintf(buf, sizeof(buf), "  SRAM Base     : 0x%08lX (Size: %lu MB)\r\n",
             (unsigned long)SOC_RAM_BASE, (unsigned long)(SOC_RAM_SIZE / (1024 * 1024)));
    diag_log_raw(buf);
    diag_log_raw("---------------------\r\n\r\n");
}

static void cmd_peek(int argc, char **argv)
{
    if (argc < 2) {
        diag_log_raw("Usage: peek <hex_addr>\r\n");
        return;
    }
    uintptr_t addr = (uintptr_t)parse_hex_or_dec(argv[1]);
    uint32_t val = mmio_read32(addr);
    char buf[64];
    snprintf(buf, sizeof(buf), "[PEEK] [0x%08lX] = 0x%08lX (%lu)\r\n",
             (unsigned long)addr, (unsigned long)val, (unsigned long)val);
    diag_log_raw(buf);
}

static void cmd_poke(int argc, char **argv)
{
    if (argc < 3) {
        diag_log_raw("Usage: poke <hex_addr> <hex_val>\r\n");
        return;
    }
    uintptr_t addr = (uintptr_t)parse_hex_or_dec(argv[1]);
    uint32_t val = parse_hex_or_dec(argv[2]);
    mmio_write32(addr, val);
    char buf[64];
    snprintf(buf, sizeof(buf), "[POKE] [0x%08lX] <= 0x%08lX\r\n",
             (unsigned long)addr, (unsigned long)val);
    diag_log_raw(buf);
}

static void cmd_dump(int argc, char **argv)
{
    if (argc < 2) {
        diag_log_raw("Usage: dump <hex_addr> [len_bytes]\r\n");
        return;
    }
    uintptr_t addr = (uintptr_t)parse_hex_or_dec(argv[1]);
    size_t len = (argc >= 3) ? (size_t)parse_hex_or_dec(argv[2]) : 64;
    if (len > 1024) len = 1024;
    diag_dump_hex("USER_DUMP", addr, (const void *)addr, len);
}

static void cmd_fill(int argc, char **argv)
{
    if (argc < 4) {
        diag_log_raw("Usage: fill <addr> <words> <val>\r\n");
        return;
    }
    uintptr_t addr = (uintptr_t)parse_hex_or_dec(argv[1]);
    size_t words = (size_t)parse_hex_or_dec(argv[2]);
    uint32_t val = parse_hex_or_dec(argv[3]);
    volatile uint32_t *p = (volatile uint32_t *)addr;
    for (size_t i = 0; i < words; ++i) {
        p[i] = val;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "[FILL] Filled %lu words at 0x%08lX with 0x%08lX\r\n",
             (unsigned long)words, (unsigned long)addr, (unsigned long)val);
    diag_log_raw(buf);
}

static void cmd_mtest(int argc, char **argv)
{
    if (argc < 3) {
        diag_log_raw("Usage: mtest <addr> <words> [all|walk1|walk0|alias|march|lfsr]\r\n");
        return;
    }
    uintptr_t addr = (uintptr_t)parse_hex_or_dec(argv[1]);
    size_t words = (size_t)parse_hex_or_dec(argv[2]);
    const char *algo = (argc >= 4) ? argv[3] : "all";

    diag_log(LOG_LEVEL_INFO, "Starting memory diagnosis at 0x%08lX (%lu words, algo: %s)...",
             (unsigned long)addr, (unsigned long)words, algo);

    test_result_t res = TEST_RESULT_PASS;
    if (strcmp(algo, "walk1") == 0 || strcmp(algo, "all") == 0) {
        res = diag_mem_walking_ones(addr);
        diag_log(LOG_LEVEL_INFO, "Walking 1s: %s", (res == TEST_RESULT_PASS) ? "PASS" : "FAIL");
    }
    if ((strcmp(algo, "walk0") == 0 || strcmp(algo, "all") == 0) && res == TEST_RESULT_PASS) {
        res = diag_mem_walking_zeros(addr);
        diag_log(LOG_LEVEL_INFO, "Walking 0s: %s", (res == TEST_RESULT_PASS) ? "PASS" : "FAIL");
    }
    if ((strcmp(algo, "alias") == 0 || strcmp(algo, "all") == 0) && res == TEST_RESULT_PASS) {
        res = diag_mem_address_bus_integrity(addr, words * sizeof(uint32_t));
        diag_log(LOG_LEVEL_INFO, "Address Aliasing: %s", (res == TEST_RESULT_PASS) ? "PASS" : "FAIL");
    }
    if ((strcmp(algo, "march") == 0 || strcmp(algo, "all") == 0) && res == TEST_RESULT_PASS) {
        res = diag_mem_march_c_minus(addr, words);
        diag_log(LOG_LEVEL_INFO, "March C- : %s", (res == TEST_RESULT_PASS) ? "PASS" : "FAIL");
    }
    if ((strcmp(algo, "lfsr") == 0 || strcmp(algo, "all") == 0) && res == TEST_RESULT_PASS) {
        res = diag_mem_lfsr_pattern(addr, words, 0x12345678UL);
        diag_log(LOG_LEVEL_INFO, "LFSR Pattern: %s", (res == TEST_RESULT_PASS) ? "PASS" : "FAIL");
    }

    if (res == TEST_RESULT_PASS) {
        diag_log_raw("[MTEST RESULT] >>> MEMORY REGION QUALIFIED: INTEGRITY VERIFIED <<<\r\n");
    } else {
        diag_log_raw("[MTEST RESULT] >>> MEMORY REGION FAILED DIAGNOSTIC <<<\r\n");
    }
}

static void cmd_gpio(int argc, char **argv)
{
    if (argc < 3) {
        diag_log_raw("Usage: gpio <read|write|toggle> <pin> [val]\r\n");
        return;
    }
    const char *action = argv[1];
    uint8_t pin = (uint8_t)parse_hex_or_dec(argv[2]);

    if (strcmp(action, "read") == 0) {
        bool state = gpio_read_pin(SOC_GPIO0_BASE, pin);
        char buf[64];
        snprintf(buf, sizeof(buf), "[GPIO] Pin %u = %s\r\n", pin, state ? "HIGH (1)" : "LOW (0)");
        diag_log_raw(buf);
    } else if (strcmp(action, "write") == 0 && argc >= 4) {
        bool state = (parse_hex_or_dec(argv[3]) != 0);
        gpio_write_pin(SOC_GPIO0_BASE, pin, state);
        char buf[64];
        snprintf(buf, sizeof(buf), "[GPIO] Pin %u set to %s\r\n", pin, state ? "HIGH (1)" : "LOW (0)");
        diag_log_raw(buf);
    } else if (strcmp(action, "toggle") == 0) {
        gpio_toggle_pin(SOC_GPIO0_BASE, pin);
        bool state = gpio_read_pin(SOC_GPIO0_BASE, pin);
        char buf[64];
        snprintf(buf, sizeof(buf), "[GPIO] Pin %u toggled -> now %s\r\n", pin, state ? "HIGH (1)" : "LOW (0)");
        diag_log_raw(buf);
    }
}

static void cmd_diag(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "all") == 0) {
        bringup_run_all();
    } else if (strcmp(argv[1], "phase") == 0 && argc >= 3) {
        bringup_phase_t phase = (bringup_phase_t)parse_hex_or_dec(argv[2]);
        bringup_run_phase(phase);
    } else if (strcmp(argv[1], "test") == 0 && argc >= 3) {
        uint32_t tid = parse_hex_or_dec(argv[2]);
        bringup_run_test_by_id(tid);
    } else {
        diag_log_raw("Usage: diag [all|phase <1-7>|test <id>]\r\n");
    }
}

int bringup_cli_execute_command(char *cmd_line)
{
    if (!cmd_line || cmd_line[0] == '\0') return 0;

    /* Tokenize into argc / argv */
    char *argv[CLI_MAX_ARGS];
    int argc = 0;
    char *token = strtok(cmd_line, " \t\r\n");

    while (token != NULL && argc < CLI_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    if (argc == 0) return 0;

    const char *cmd = argv[0];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        cmd_help(argc, argv);
    } else if (strcmp(cmd, "version") == 0) {
        cmd_version(argc, argv);
    } else if (strcmp(cmd, "status") == 0) {
        cmd_status(argc, argv);
    } else if (strcmp(cmd, "peek") == 0) {
        cmd_peek(argc, argv);
    } else if (strcmp(cmd, "poke") == 0) {
        cmd_poke(argc, argv);
    } else if (strcmp(cmd, "dump") == 0) {
        cmd_dump(argc, argv);
    } else if (strcmp(cmd, "fill") == 0) {
        cmd_fill(argc, argv);
    } else if (strcmp(cmd, "mtest") == 0) {
        cmd_mtest(argc, argv);
    } else if (strcmp(cmd, "gpio") == 0) {
        cmd_gpio(argc, argv);
    } else if (strcmp(cmd, "diag") == 0) {
        cmd_diag(argc, argv);
    } else if (strcmp(cmd, "reboot") == 0) {
        diag_log_raw("[SYSTEM] Initiating software reset...\r\n");
#if defined(ARCH_ARM)
        /* ARM AIRCR System Reset */
        mmio_write32(SOC_SCB_BASE + 0x0C, 0x05FA0004);
#endif
    } else {
        diag_log_raw("Unknown command. Type 'help' for command list.\r\n");
        return -1;
    }

    return 0;
}

void bringup_cli_init(void)
{
    s_line_idx = 0;
    diag_log_raw("\r\n==================================================================\r\n");
    diag_log_raw("  BARE-METAL SILICON BRINGUP CONSOLE - TYPE 'help' OR 'diag all'\r\n");
    diag_log_raw("==================================================================\r\n");
    diag_log_raw("bringup> ");
}

void bringup_cli_poll(void)
{
    while (uart_has_rx(SOC_UART0_BASE)) {
        char c = uart_getc(SOC_UART0_BASE);
        if (c == '\r' || c == '\n') {
            diag_log_raw("\r\n");
            s_line_buf[s_line_idx] = '\0';
            bringup_cli_execute_command(s_line_buf);
            s_line_idx = 0;
            diag_log_raw("bringup> ");
        } else if (c == '\b' || c == 0x7F) {
            if (s_line_idx > 0) {
                s_line_idx--;
                diag_log_raw("\b \b");
            }
        } else if (s_line_idx < (CLI_LINE_MAX - 1)) {
            s_line_buf[s_line_idx++] = c;
            uart_putc(SOC_UART0_BASE, c); /* Echo */
        }
    }
}

void bringup_cli_run_interactive(void)
{
    bringup_cli_init();
    while (1) {
        bringup_cli_poll();
    }
}

