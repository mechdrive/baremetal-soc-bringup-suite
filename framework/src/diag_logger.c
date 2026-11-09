/**
 * @file diag_logger.c
 * @brief Silicon Diagnostic Logging & Hex Memory Dump Engine
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "bringup_framework.h"
#include "uart.h"
#include "timer.h"
#include "soc_regs.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_BUF_SIZE 256

void diag_log_raw(const char *str)
{
    if (!str) return;
    uart_puts(SOC_UART0_BASE, str);
}

void diag_log(log_level_t level, const char *fmt, ...)
{
    char msg[LOG_BUF_SIZE];
    char final_buf[LOG_BUF_SIZE + 64];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    uint32_t ms = timer_get_time_ms();
    const char *lvl_tag = "INFO ";

    switch (level) {
        case LOG_LEVEL_DEBUG: lvl_tag = "DEBUG"; break;
        case LOG_LEVEL_INFO:  lvl_tag = "INFO "; break;
        case LOG_LEVEL_WARN:  lvl_tag = "WARN "; break;
        case LOG_LEVEL_ERROR: lvl_tag = "ERROR"; break;
        case LOG_LEVEL_FATAL: lvl_tag = "FATAL"; break;
    }

    snprintf(final_buf, sizeof(final_buf), "[%06lu ms] [%s] %s\r\n", (unsigned long)ms, lvl_tag, msg);
    diag_log_raw(final_buf);
}

void diag_dump_hex(const char *tag, uintptr_t addr, const void *data, size_t len)
{
    if (!data || len == 0) return;

    char line[128];
    const uint8_t *p = (const uint8_t *)data;

    if (tag) {
        snprintf(line, sizeof(line), "--- Memory Dump: %s (Base: 0x%08lX, Length: %lu bytes) ---\r\n",
                 tag, (unsigned long)addr, (unsigned long)len);
        diag_log_raw(line);
    }

    for (size_t i = 0; i < len; i += 16) {
        size_t chunk = (len - i >= 16) ? 16 : (len - i);
        int offset = snprintf(line, sizeof(line), "0x%08lX: ", (unsigned long)(addr + i));

        /* Hex bytes */
        for (size_t j = 0; j < 16; ++j) {
            if (j < chunk) {
                offset += snprintf(line + offset, sizeof(line) - offset, "%02X ", p[i + j]);
            } else {
                offset += snprintf(line + offset, sizeof(line) - offset, "   ");
            }
            if (j == 7) {
                offset += snprintf(line + offset, sizeof(line) - offset, " ");
            }
        }

        offset += snprintf(line + offset, sizeof(line) - offset, " |");

        /* ASCII representation */
        for (size_t j = 0; j < chunk; ++j) {
            char c = (char)p[i + j];
            if (c < 32 || c > 126) {
                c = '.';
            }
            offset += snprintf(line + offset, sizeof(line) - offset, "%c", c);
        }
        snprintf(line + offset, sizeof(line) - offset, "|\r\n");
        diag_log_raw(line);
    }
}

