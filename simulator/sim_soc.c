/**
 * @file sim_soc.c
 * @brief Virtual Silicon Bus & Peripheral Simulation Engine Implementation
 *
 * Provides register-accurate simulation for ARM Cortex-M and RISC-V peripherals,
 * memory spaces, bus arbitration, and communication interfaces.
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "sim_soc.h"
#include "soc_regs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIM_RAM_SIZE (2 * 1024 * 1024UL) /* 2 MB virtual SRAM */

/* Virtual Storage Banks */
static uint8_t *s_sim_ram = NULL;

/* Virtual Peripheral Registers */
static uint32_t s_rcc_regs[64];
static uint32_t s_timer_regs[16];
static uint32_t s_gpio_regs[16];
static uint32_t s_spi_regs[16];
static uint32_t s_i2c_regs[16];
static uint32_t s_wdt_regs[16];
static uint32_t s_dma_regs[64];
static uint32_t s_nvic_regs[64];
static uint32_t s_mpu_regs[32];

/* UART Loopback FIFO */
#define UART_FIFO_CAPACITY 256
static uint8_t s_uart_rx_fifo[UART_FIFO_CAPACITY];
static size_t s_uart_rx_head = 0;
static size_t s_uart_rx_tail = 0;
static size_t s_uart_rx_count = 0;
static uint32_t s_uart_cr = 0;
static uint32_t s_uart_fr = UART_FR_TXFE | UART_FR_RXFE;

/* I2C Emulated EEPROM */
static uint8_t s_i2c_eeprom[256];
static uint8_t s_i2c_addr_ptr = 0;
static uint8_t s_i2c_rx_byte = 0;

/* Virtual timekeeping */
static uint64_t s_sim_start_time_us = 0;

static uint64_t get_host_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000ULL) + (ts.tv_nsec / 1000ULL);
}

void sim_soc_init(void)
{
    if (!s_sim_ram) {
        s_sim_ram = (uint8_t *)calloc(1, SIM_RAM_SIZE);
    }
    memset(s_rcc_regs, 0, sizeof(s_rcc_regs));
    memset(s_timer_regs, 0, sizeof(s_timer_regs));
    memset(s_gpio_regs, 0, sizeof(s_gpio_regs));
    memset(s_spi_regs, 0, sizeof(s_spi_regs));
    memset(s_i2c_regs, 0, sizeof(s_i2c_regs));
    memset(s_wdt_regs, 0, sizeof(s_wdt_regs));
    memset(s_dma_regs, 0, sizeof(s_dma_regs));
    memset(s_nvic_regs, 0, sizeof(s_nvic_regs));
    memset(s_mpu_regs, 0, sizeof(s_mpu_regs));
    memset(s_i2c_eeprom, 0, sizeof(s_i2c_eeprom));

    /* Initialize default register states */
    s_rcc_regs[RCC_CSR_OFFSET / 4] = RCC_CSR_PORRSTF; /* Reset cause: Power-On Reset */
    s_uart_cr = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
    s_uart_fr = UART_FR_TXFE | UART_FR_RXFE;
    s_wdt_regs[WDT_VALUE_OFFSET / 4] = 2500000;

    s_sim_start_time_us = get_host_us();
}

void sim_soc_destroy(void)
{
    if (s_sim_ram) {
        free(s_sim_ram);
        s_sim_ram = NULL;
    }
}

/* -------------------------------------------------------------------------- */
/* Simulated MMIO Dispatcher                                                  */
/* -------------------------------------------------------------------------- */

uint32_t sim_mmio_read32(uintptr_t addr)
{
    if (!s_sim_ram) {
        sim_soc_init();
    }

    /* 1. Virtual SRAM Mapping */
    if (addr >= SOC_RAM_BASE && addr < (SOC_RAM_BASE + SIM_RAM_SIZE)) {
        uintptr_t offset = addr - SOC_RAM_BASE;
        return *(uint32_t *)&s_sim_ram[offset];
    }

    /* 2. Primary UART */
    if (addr >= SOC_UART0_BASE && addr < (SOC_UART0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_UART0_BASE);
        if (off == UART_DR_OFFSET) {
            if (s_uart_rx_count > 0) {
                uint8_t byte = s_uart_rx_fifo[s_uart_rx_tail];
                s_uart_rx_tail = (s_uart_rx_tail + 1) % UART_FIFO_CAPACITY;
                s_uart_rx_count--;
                if (s_uart_rx_count == 0) {
                    s_uart_fr |= UART_FR_RXFE;
                }
                s_uart_fr &= ~UART_FR_RXFF;
                return byte;
            }
            return 0;
        } else if (off == UART_FR_OFFSET) {
            return s_uart_fr;
        } else if (off == UART_CR_OFFSET) {
            return s_uart_cr;
        }
        return 0;
    }

    /* 3. System Timer */
    if (addr >= SOC_TIMER0_BASE && addr < (SOC_TIMER0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_TIMER0_BASE);
        if (off == TIMER_VALUE_OFFSET) {
            /* Synthesize decrementing timer tick */
            uint64_t now = get_host_us() - s_sim_start_time_us;
            uint32_t reload = s_timer_regs[TIMER_RELOAD_OFFSET / 4];
            if (reload == 0) reload = 25000;
            return (uint32_t)(reload - (now % reload));
        }
        return s_timer_regs[off / 4];
    }

    /* 4. Reset & Clock Controller (RCC) */
    if (addr >= SOC_RCC_BASE && addr < (SOC_RCC_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_RCC_BASE);
        return s_rcc_regs[off / 4];
    }

    /* 5. GPIO Port 0 */
    if (addr >= SOC_GPIO0_BASE && addr < (SOC_GPIO0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_GPIO0_BASE);
        return s_gpio_regs[off / 4];
    }

    /* 6. SPI0 Controller */
    if (addr >= SOC_SPI0_BASE && addr < (SOC_SPI0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_SPI0_BASE);
        if (off == SPI_SR_OFFSET) {
            return s_spi_regs[off / 4] | SPI_SR_TXE | SPI_SR_RXNE;
        }
        return s_spi_regs[off / 4];
    }

    /* 7. I2C0 Controller */
    if (addr >= SOC_I2C0_BASE && addr < (SOC_I2C0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_I2C0_BASE);
        if (off == I2C_DR_OFFSET) {
            return s_i2c_rx_byte;
        }
        return s_i2c_regs[off / 4];
    }

    /* 8. Watchdog Timer */
    if (addr >= SOC_WDT_BASE && addr < (SOC_WDT_BASE + 0x1000)) {
        uint32_t off = (uint32_t)(addr - SOC_WDT_BASE);
        if (off == WDT_VALUE_OFFSET) {
            if (s_wdt_regs[WDT_VALUE_OFFSET / 4] > 10) {
                s_wdt_regs[WDT_VALUE_OFFSET / 4] -= 10;
            }
            return s_wdt_regs[WDT_VALUE_OFFSET / 4];
        }
        return s_wdt_regs[off / 4];
    }

    /* 9. DMA Controller */
    if (addr >= SOC_DMA0_BASE && addr < (SOC_DMA0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_DMA0_BASE);
        return s_dma_regs[off / 4];
    }

    /* 10. NVIC & Core SCS */
    if (addr >= SOC_NVIC_BASE && addr < (SOC_NVIC_BASE + 0x1000)) {
        uint32_t off = (uint32_t)(addr - SOC_NVIC_BASE);
        return s_nvic_regs[(off / 4) % 64];
    }

    /* 11. MPU Base */
    if (addr >= SOC_MPU_BASE && addr < (SOC_MPU_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_MPU_BASE);
        return s_mpu_regs[(off / 4) % 32];
    }

    /* 12. Host Memory Pointer Fallback */
    return *(volatile uint32_t *)addr;
}

void sim_mmio_write32(uintptr_t addr, uint32_t val)
{
    if (!s_sim_ram) {
        sim_soc_init();
    }

    /* 1. Virtual SRAM Mapping */
    if (addr >= SOC_RAM_BASE && addr < (SOC_RAM_BASE + SIM_RAM_SIZE)) {
        uintptr_t offset = addr - SOC_RAM_BASE;
        *(uint32_t *)&s_sim_ram[offset] = val;
        return;
    }

    /* 2. Primary UART */
    if (addr >= SOC_UART0_BASE && addr < (SOC_UART0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_UART0_BASE);
        if (off == UART_DR_OFFSET) {
            char c = (char)(val & 0xFF);
            fputc(c, stdout);
            fflush(stdout);

            /* If loopback mode active, enqueue to RX FIFO */
            if (s_uart_cr & UART_CR_LBE) {
                if (s_uart_rx_count < UART_FIFO_CAPACITY) {
                    s_uart_rx_fifo[s_uart_rx_head] = (uint8_t)c;
                    s_uart_rx_head = (s_uart_rx_head + 1) % UART_FIFO_CAPACITY;
                    s_uart_rx_count++;
                    s_uart_fr &= ~UART_FR_RXFE;
                    if (s_uart_rx_count >= UART_FIFO_CAPACITY) {
                        s_uart_fr |= UART_FR_RXFF;
                    }
                }
            }
        } else if (off == UART_CR_OFFSET) {
            s_uart_cr = val;
        }
        return;
    }

    /* 3. System Timer */
    if (addr >= SOC_TIMER0_BASE && addr < (SOC_TIMER0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_TIMER0_BASE);
        s_timer_regs[off / 4] = val;
        return;
    }

    /* 4. Reset & Clock Controller (RCC) */
    if (addr >= SOC_RCC_BASE && addr < (SOC_RCC_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_RCC_BASE);
        s_rcc_regs[off / 4] = val;

        /* Auto-assert ready flags when oscillators or PLL enabled */
        if (off == RCC_CR_OFFSET) {
            if (val & RCC_CR_HSION) s_rcc_regs[RCC_CR_OFFSET / 4] |= RCC_CR_HSIRDY;
            if (val & RCC_CR_HSEON) s_rcc_regs[RCC_CR_OFFSET / 4] |= RCC_CR_HSERDY;
            if (val & RCC_CR_PLLON) s_rcc_regs[RCC_CR_OFFSET / 4] |= RCC_CR_PLLRDY;
        } else if (off == RCC_CSR_OFFSET) {
            if (val & RCC_CSR_RMVF) {
                s_rcc_regs[RCC_CSR_OFFSET / 4] &= ~0xFF000000UL; /* Clear reset flags */
            }
        }
        return;
    }

    /* 5. GPIO Port 0 */
    if (addr >= SOC_GPIO0_BASE && addr < (SOC_GPIO0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_GPIO0_BASE);
        if (off == GPIO_BSRR_OFFSET) {
            /* Atomic Bit Set/Reset */
            uint16_t set_bits = (uint16_t)(val & 0xFFFF);
            uint16_t clr_bits = (uint16_t)((val >> 16) & 0xFFFF);
            s_gpio_regs[GPIO_ODR_OFFSET / 4] = (s_gpio_regs[GPIO_ODR_OFFSET / 4] | set_bits) & ~clr_bits;
            s_gpio_regs[GPIO_IDR_OFFSET / 4] = s_gpio_regs[GPIO_ODR_OFFSET / 4];
        } else if (off == GPIO_ODR_OFFSET) {
            s_gpio_regs[GPIO_ODR_OFFSET / 4] = val;
            s_gpio_regs[GPIO_IDR_OFFSET / 4] = val;
        } else {
            s_gpio_regs[off / 4] = val;
        }
        return;
    }

    /* 6. SPI0 Controller */
    if (addr >= SOC_SPI0_BASE && addr < (SOC_SPI0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_SPI0_BASE);
        s_spi_regs[off / 4] = val;
        if (off == SPI_DR_OFFSET) {
            /* In loopback, write to DR immediately updates readback DR */
            s_spi_regs[SPI_DR_OFFSET / 4] = val;
            s_spi_regs[SPI_SR_OFFSET / 4] |= (SPI_SR_RXNE | SPI_SR_TXE);
        }
        return;
    }

    /* 7. I2C0 Controller */
    if (addr >= SOC_I2C0_BASE && addr < (SOC_I2C0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_I2C0_BASE);
        s_i2c_regs[off / 4] = val;

        if (off == I2C_CR1_OFFSET) {
            if (val & I2C_CR1_START) {
                s_i2c_regs[I2C_SR1_OFFSET / 4] |= I2C_SR1_SB;
            }
        } else if (off == I2C_DR_OFFSET) {
            uint8_t byte = (uint8_t)(val & 0xFF);
            if (s_i2c_regs[I2C_SR1_OFFSET / 4] & I2C_SR1_SB) {
                /* Address phase */
                s_i2c_regs[I2C_SR1_OFFSET / 4] &= ~I2C_SR1_SB;
                uint8_t dev_addr = byte >> 1;
                if (dev_addr == 0x50) {
                    s_i2c_regs[I2C_SR1_OFFSET / 4] |= I2C_SR1_ADDR;
                    if (byte & 0x01) {
                        /* Read mode */
                        s_i2c_rx_byte = s_i2c_eeprom[s_i2c_addr_ptr++];
                        s_i2c_regs[I2C_SR1_OFFSET / 4] |= I2C_SR1_RXNE;
                    }
                } else {
                    s_i2c_regs[I2C_SR1_OFFSET / 4] |= I2C_SR1_AF; /* NACK */
                }
            } else {
                /* Data phase */
                s_i2c_eeprom[s_i2c_addr_ptr++] = byte;
                s_i2c_regs[I2C_SR1_OFFSET / 4] |= (I2C_SR1_TXE | I2C_SR1_BTF);
            }
        }
        return;
    }

    /* 8. Watchdog Timer */
    if (addr >= SOC_WDT_BASE && addr < (SOC_WDT_BASE + 0x1000)) {
        uint32_t off = (uint32_t)(addr - SOC_WDT_BASE);
        s_wdt_regs[off / 4] = val;
        if (off == WDT_INTCLR_OFFSET) {
            /* Reset counter to load value */
            s_wdt_regs[WDT_VALUE_OFFSET / 4] = s_wdt_regs[WDT_LOAD_OFFSET / 4];
        }
        return;
    }

    /* 9. DMA Controller */
    if (addr >= SOC_DMA0_BASE && addr < (SOC_DMA0_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_DMA0_BASE);
        s_dma_regs[off / 4] = val;

        /* If Channel 0 or 1 enabled, execute simulated block copy */
        for (int ch = 0; ch < 2; ++ch) {
            uint32_t ch_ctrl_idx = (DMA_CH0_CTRL_OFFSET + (ch * 0x20)) / 4;
            if (s_dma_regs[ch_ctrl_idx] & DMA_CTRL_EN) {
                uintptr_t src = s_dma_regs[ch_ctrl_idx + 1];
                uintptr_t dst = s_dma_regs[ch_ctrl_idx + 2];
                size_t cnt    = s_dma_regs[ch_ctrl_idx + 3];

                /* Perform copy directly through sim_mmio */
                for (size_t b = 0; b < cnt; ++b) {
                    sim_mmio_write8(dst + b, sim_mmio_read8(src + b));
                }

                s_dma_regs[ch_ctrl_idx + 4] = DMA_STATUS_COMPLETE;
                s_dma_regs[ch_ctrl_idx] &= ~DMA_CTRL_EN; /* Clear enable on completion */
            }
        }
        return;
    }

    /* 10. NVIC */
    if (addr >= SOC_NVIC_BASE && addr < (SOC_NVIC_BASE + 0x1000)) {
        uint32_t off = (uint32_t)(addr - SOC_NVIC_BASE);
        uint32_t idx = (off / 4) % 64;
        if (off >= 0x000 && off < 0x020) {
            s_nvic_regs[idx] |= val; /* ISER */
        } else if (off >= 0x080 && off < 0x0A0) {
            s_nvic_regs[idx - 0x20] &= ~val; /* ICER */
        } else {
            s_nvic_regs[idx] = val;
        }
        return;
    }

    /* 11. MPU */
    if (addr >= SOC_MPU_BASE && addr < (SOC_MPU_BASE + 0x100)) {
        uint32_t off = (uint32_t)(addr - SOC_MPU_BASE);
        s_mpu_regs[(off / 4) % 32] = val;
        return;
    }

    /* 12. Host Memory Pointer Fallback */
    *(volatile uint32_t *)addr = val;
}

uint16_t sim_mmio_read16(uintptr_t addr)
{
    uint32_t val32 = sim_mmio_read32(addr & ~3UL);
    uint32_t shift = (addr & 2UL) ? 16 : 0;
    return (uint16_t)((val32 >> shift) & 0xFFFF);
}

void sim_mmio_write16(uintptr_t addr, uint16_t val)
{
    uint32_t word = sim_mmio_read32(addr & ~3UL);
    if (addr & 2UL) {
        word = (word & 0x0000FFFFUL) | ((uint32_t)val << 16);
    } else {
        word = (word & 0xFFFF0000UL) | val;
    }
    sim_mmio_write32(addr & ~3UL, word);
}

uint8_t sim_mmio_read8(uintptr_t addr)
{
    uint32_t val32 = sim_mmio_read32(addr & ~3UL);
    uint32_t shift = (addr & 3UL) * 8;
    return (uint8_t)((val32 >> shift) & 0xFF);
}

void sim_mmio_write8(uintptr_t addr, uint8_t val)
{
    uint32_t word = sim_mmio_read32(addr & ~3UL);
    uint32_t shift = (addr & 3UL) * 8;
    uint32_t mask = 0xFFUL << shift;
    word = (word & ~mask) | ((uint32_t)val << shift);
    sim_mmio_write32(addr & ~3UL, word);
}
