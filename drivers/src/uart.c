/**
 * @file uart.c
 * @brief Universal SoC UART Driver Implementation
 *
 * Implements register-level serial communication supporting both ARM PL011/CMSDK
 * and NS16550A compatible architectures.
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "uart.h"
#include "mmio.h"
#include "soc_regs.h"

int uart_init(uintptr_t base_addr, const uart_config_t *config)
{
    if (base_addr == 0 || config == 0) {
        return -1;
    }

#if defined(ARCH_RISCV)
    /* NS16550A UART Initialization for RISC-V platforms */
    /* Disable all interrupts: IER = 0 */
    mmio_write8(base_addr + 1, 0x00);
    
    /* Enable DLAB (Divisor Latch Access Bit) to configure baud rate: LCR = 0x80 */
    mmio_write8(base_addr + 3, 0x80);
    
    /* For 38400 baud with 1.8432MHz or 115200 with standard clock: set divisor */
    uint16_t divisor = 1;
    if (config->baudrate == 115200) {
        divisor = 1;
    } else if (config->baudrate == 9600) {
        divisor = 12;
    }
    mmio_write8(base_addr + 0, (uint8_t)(divisor & 0xFF));
    mmio_write8(base_addr + 1, (uint8_t)((divisor >> 8) & 0xFF));
    
    /* Set 8 data bits, 1 stop bit, no parity: LCR = 0x03 */
    uint8_t lcr = 0x03;
    if (config->stop_bits == UART_STOP_2) {
        lcr |= 0x04;
    }
    if (config->parity == UART_PARITY_EVEN) {
        lcr |= 0x18;
    } else if (config->parity == UART_PARITY_ODD) {
        lcr |= 0x08;
    }
    mmio_write8(base_addr + 3, lcr);
    
    /* Enable and clear FIFOs with 14-byte trigger level: FCR = 0xC7 */
    if (config->enable_fifo) {
        mmio_write8(base_addr + 2, 0xC7);
    } else {
        mmio_write8(base_addr + 2, 0x00);
    }
    
    /* Set DTR/RTS and optionally enable loopback in MCR */
    uint8_t mcr = 0x03;
    if (config->loopback_mode) {
        mcr |= 0x10; /* Loopback bit in 16550 MCR */
    }
    mmio_write8(base_addr + 4, mcr);

#else
    /* ARM PL011 / Universal Register Interface */
    /* 1. Disable UART during configuration */
    mmio_write32(base_addr + UART_CR_OFFSET, 0);

    /* 2. Clear any pending interrupts */
    mmio_write32(base_addr + UART_ICR_OFFSET, 0x7FF);

    /* 3. Configure Baud Rate: Assume 24MHz reference clock */
    /* Divisor = 24000000 / (16 * baudrate) */
    uint32_t baud = config->baudrate ? config->baudrate : 115200;
    uint32_t ibrd = 24000000 / (16 * baud);
    if (ibrd == 0) ibrd = 1;
    uint32_t fbrd = ((24000000 % (16 * baud)) * 64 + (8 * baud)) / (16 * baud);
    
    mmio_write32(base_addr + UART_IBRD_OFFSET, ibrd);
    mmio_write32(base_addr + UART_FBRD_OFFSET, fbrd);

    /* 4. Configure Line Control (Word length, FIFOs, Parity, Stop bits) */
    uint32_t lcrh = UART_LCRH_WLEN_8;
    if (config->enable_fifo) {
        lcrh |= UART_LCRH_FEN;
    }
    if (config->stop_bits == UART_STOP_2) {
        lcrh |= UART_LCRH_STP2;
    }
    if (config->parity == UART_PARITY_EVEN) {
        lcrh |= (UART_LCRH_PEN | UART_LCRH_EPS);
    } else if (config->parity == UART_PARITY_ODD) {
        lcrh |= UART_LCRH_PEN;
    }
    mmio_write32(base_addr + UART_LCRH_OFFSET, lcrh);

    /* 5. Enable TX, RX, UART, and optionally Loopback */
    uint32_t cr = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
    if (config->loopback_mode) {
        cr |= UART_CR_LBE;
    }
    mmio_write32(base_addr + UART_CR_OFFSET, cr);
#endif

    return 0;
}

void uart_putc(uintptr_t base_addr, char c)
{
#if defined(ARCH_RISCV)
    /* Poll Line Status Register (LSR, offset 5) bit 5: Transmit Holding Register Empty */
    while ((mmio_read8(base_addr + 5) & 0x20) == 0) {
        /* Busy wait */
    }
    mmio_write8(base_addr + 0, (uint8_t)c);
#else
    /* Poll Flag Register (FR) bit 5: TXFF (TX FIFO Full) */
    while ((mmio_read32(base_addr + UART_FR_OFFSET) & UART_FR_TXFF) != 0) {
        /* Busy wait */
    }
    mmio_write32(base_addr + UART_DR_OFFSET, (uint32_t)c);
#endif
}

char uart_getc(uintptr_t base_addr)
{
#if defined(ARCH_RISCV)
    /* Poll LSR bit 0: Data Ready */
    while ((mmio_read8(base_addr + 5) & 0x01) == 0) {
        /* Busy wait */
    }
    return (char)mmio_read8(base_addr + 0);
#else
    /* Poll FR bit 4: RXFE (RX FIFO Empty) */
    while ((mmio_read32(base_addr + UART_FR_OFFSET) & UART_FR_RXFE) != 0) {
        /* Busy wait */
    }
    return (char)(mmio_read32(base_addr + UART_DR_OFFSET) & 0xFF);
#endif
}

bool uart_has_rx(uintptr_t base_addr)
{
#if defined(ARCH_RISCV)
    return (mmio_read8(base_addr + 5) & 0x01) != 0;
#else
    return (mmio_read32(base_addr + UART_FR_OFFSET) & UART_FR_RXFE) == 0;
#endif
}

bool uart_can_tx(uintptr_t base_addr)
{
#if defined(ARCH_RISCV)
    return (mmio_read8(base_addr + 5) & 0x20) != 0;
#else
    return (mmio_read32(base_addr + UART_FR_OFFSET) & UART_FR_TXFF) == 0;
#endif
}

void uart_puts(uintptr_t base_addr, const char *str)
{
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            uart_putc(base_addr, '\r');
        }
        uart_putc(base_addr, *str++);
    }
}

void uart_write(uintptr_t base_addr, const uint8_t *buf, size_t len)
{
    if (!buf) return;
    for (size_t i = 0; i < len; ++i) {
        uart_putc(base_addr, (char)buf[i]);
    }
}

size_t uart_read(uintptr_t base_addr, uint8_t *buf, size_t max_len, uint32_t timeout_loops)
{
    if (!buf || max_len == 0) return 0;
    size_t count = 0;

    while (count < max_len) {
        uint32_t loops = 0;
        while (!uart_has_rx(base_addr)) {
            if (timeout_loops > 0 && ++loops >= timeout_loops) {
                return count;
            }
        }
        buf[count++] = (uint8_t)uart_getc(base_addr);
    }
    return count;
}

void uart_set_loopback(uintptr_t base_addr, bool enable)
{
#if defined(ARCH_RISCV)
    uint8_t mcr = mmio_read8(base_addr + 4);
    if (enable) {
        mcr |= 0x10;
    } else {
        mcr &= ~0x10;
    }
    mmio_write8(base_addr + 4, mcr);
#else
    uint32_t cr = mmio_read32(base_addr + UART_CR_OFFSET);
    if (enable) {
        cr |= UART_CR_LBE;
    } else {
        cr &= ~UART_CR_LBE;
    }
    mmio_write32(base_addr + UART_CR_OFFSET, cr);
#endif
}

