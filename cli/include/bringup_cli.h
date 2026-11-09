/**
 * @file bringup_cli.h
 * @brief Interactive Silicon Bringup UART Monitor / Shell
 *
 * Provides real-time register peek/poke, memory inspection, diagnostic runner,
 * and peripheral control commands over serial console.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef BRINGUP_CLI_H
#define BRINGUP_CLI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the bringup CLI subsystem and display banner.
 */
void bringup_cli_init(void);

/**
 * @brief Process single line or non-blocking character inputs.
 */
void bringup_cli_poll(void);

/**
 * @brief Execute a raw text command string directly.
 * @param cmd_line Command line string
 * @return 0 on success, negative on syntax or execution error
 */
int bringup_cli_execute_command(char *cmd_line);

/**
 * @brief Enter blocking interactive console loop.
 */
void bringup_cli_run_interactive(void);

#ifdef __cplusplus
}
#endif

#endif /* BRINGUP_CLI_H */

