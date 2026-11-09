/**
 * @file soc_regs.h
 * @brief Universal SoC Peripheral Register Maps & Base Addresses
 *
 * Covers standard memory maps for ARM Cortex-M (MPS2 / LM3S / STM32-style)
 * and RISC-V (Virt / SiFive-style), along with common peripheral register offsets.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef SOC_REGS_H
#define SOC_REGS_H

#include <stdint.h>

/* ========================================================================== */
/* Platform Selection & Base Memory Layouts                                   */
/* ========================================================================== */

#if defined(ARCH_RISCV)

    /* RISC-V QEMU Virt Platform Base Addresses */
    #define SOC_ROM_BASE            0x00001000UL
    #define SOC_RAM_BASE            0x80000000UL
    #define SOC_RAM_SIZE            (128 * 1024 * 1024UL) /* 128 MB */
    
    #define SOC_CLINT_BASE          0x02000000UL
    #define SOC_PLIC_BASE           0x0C000000UL
    #define SOC_UART0_BASE          0x10000000UL /* NS16550A compatible */
    #define SOC_RTC_BASE            0x00101000UL
    #define SOC_TEST_BASE           0x00100000UL /* QEMU test / poweroff device */

    /* Generic Peripheral Base Mappings for RISC-V Diagnostic Suite */
    #define SOC_RCC_BASE            0x10010000UL
    #define SOC_GPIO0_BASE          0x10012000UL
    #define SOC_SPI0_BASE           0x10014000UL
    #define SOC_I2C0_BASE           0x10016000UL
    #define SOC_WDT_BASE            0x10018000UL
    #define SOC_DMA0_BASE           0x1001A000UL
    #define SOC_TIMER0_BASE         0x1001C000UL

#else /* Default: ARM Cortex-M Platform (MPS2 / Cortex-M3 / Cortex-M4) */

    #define SOC_ROM_BASE            0x00000000UL
    #define SOC_ROM_SIZE            (4 * 1024 * 1024UL)   /* 4 MB Flash */
    #define SOC_RAM_BASE            0x20000000UL
    #define SOC_RAM_SIZE            (16 * 1024 * 1024UL)  /* 16 MB SRAM */
    
    /* Core ARM System Control & NVIC */
    #define SOC_SCS_BASE            0xE000E000UL
    #define SOC_SYSTICK_BASE        (SOC_SCS_BASE + 0x0010UL)
    #define SOC_NVIC_BASE           (SOC_SCS_BASE + 0x0100UL)
    #define SOC_SCB_BASE            (SOC_SCS_BASE + 0x0D00UL)
    #define SOC_MPU_BASE            (SOC_SCS_BASE + 0x0D90UL)

    /* MPS2 / LM3S Peripheral Base Addresses */
    #define SOC_UART0_BASE          0x40004000UL /* PL011 / CMSDK UART */
    #define SOC_UART1_BASE          0x40005000UL
    #define SOC_TIMER0_BASE         0x40000000UL /* CMSDK Timer 0 */
    #define SOC_TIMER1_BASE         0x40001000UL /* CMSDK Timer 1 */
    #define SOC_WDT_BASE            0x40008000UL /* Watchdog */
    #define SOC_GPIO0_BASE          0x40010000UL /* GPIO Port 0 */
    #define SOC_GPIO1_BASE          0x40011000UL /* GPIO Port 1 */
    #define SOC_SPI0_BASE           0x40020000UL /* SPI / SSP */
    #define SOC_I2C0_BASE           0x40022000UL /* I2C Master */
    #define SOC_DMA0_BASE           0x40030000UL /* DMA-330 / PL230 DMA */
    #define SOC_RCC_BASE            0x400FE000UL /* System Control / RCC */

#endif

/* ========================================================================== */
/* Reset and Clock Control (RCC) Register Map                                 */
/* ========================================================================== */
#define RCC_CR_OFFSET           0x00U /* Clock Control Register */
#define RCC_CFGR_OFFSET         0x04U /* Clock Configuration Register */
#define RCC_PLLCFGR_OFFSET      0x08U /* PLL Configuration Register */
#define RCC_AHBENR_OFFSET       0x14U /* AHB Peripheral Clock Enable */
#define RCC_APB1ENR_OFFSET      0x1CU /* APB1 Peripheral Clock Enable */
#define RCC_APB2ENR_OFFSET      0x20U /* APB2 Peripheral Clock Enable */
#define RCC_CSR_OFFSET          0x74U /* Control & Status Register */

/* RCC_CR Bitfields */
#define RCC_CR_HSION            (1U << 0)
#define RCC_CR_HSIRDY           (1U << 1)
#define RCC_CR_HSEON            (1U << 16)
#define RCC_CR_HSERDY           (1U << 17)
#define RCC_CR_HSEBYP           (1U << 18)
#define RCC_CR_CSSON            (1U << 19)
#define RCC_CR_PLLON            (1U << 24)
#define RCC_CR_PLLRDY           (1U << 25)

/* RCC_CSR Bitfields (Reset Reason Capture) */
#define RCC_CSR_LSION           (1U << 0)
#define RCC_CSR_LSIRDY          (1U << 1)
#define RCC_CSR_RMVF            (1U << 24) /* Remove reset flag */
#define RCC_CSR_BORRSTF         (1U << 25) /* Brown-out reset flag */
#define RCC_CSR_PINRSTF         (1U << 26) /* NRST pin reset flag */
#define RCC_CSR_PORRSTF         (1U << 27) /* Power-on / Power-down reset flag */
#define RCC_CSR_SFTRSTF         (1U << 28) /* Software reset flag */
#define RCC_CSR_IWDGRSTF        (1U << 29) /* Independent watchdog reset flag */
#define RCC_CSR_WWDGRSTF        (1U << 30) /* Window watchdog reset flag */
#define RCC_CSR_LPWRRSTF        (1U << 31) /* Low-power reset flag */

/* ========================================================================== */
/* UART Register Map (PL011 / 16550 / Universal Register Mapping)             */
/* ========================================================================== */
#define UART_DR_OFFSET          0x00U /* Data Register */
#define UART_RSR_OFFSET         0x04U /* Receive Status / Error Clear */
#define UART_FR_OFFSET          0x18U /* Flag Register */
#define UART_IBRD_OFFSET        0x24U /* Integer Baud Rate Divisor */
#define UART_FBRD_OFFSET        0x28U /* Fractional Baud Rate Divisor */
#define UART_LCRH_OFFSET        0x2CU /* Line Control Register */
#define UART_CR_OFFSET          0x30U /* Control Register */
#define UART_IFLS_OFFSET        0x34U /* Interrupt FIFO Level Select */
#define UART_IMSC_OFFSET        0x38U /* Interrupt Mask Set/Clear */
#define UART_RIS_OFFSET         0x3CU /* Raw Interrupt Status */
#define UART_MIS_OFFSET         0x40U /* Masked Interrupt Status */
#define UART_ICR_OFFSET         0x44U /* Interrupt Clear Register */

/* UART_FR Flag Register Bits */
#define UART_FR_CTS             (1U << 0)
#define UART_FR_DSR             (1U << 1)
#define UART_FR_DCD             (1U << 2)
#define UART_FR_BUSY            (1U << 3)
#define UART_FR_RXFE            (1U << 4) /* RX FIFO Empty */
#define UART_FR_TXFF            (1U << 5) /* TX FIFO Full */
#define UART_FR_RXFF            (1U << 6) /* RX FIFO Full */
#define UART_FR_TXFE            (1U << 7) /* TX FIFO Empty */

/* UART_CR Control Register Bits */
#define UART_CR_UARTEN          (1U << 0) /* Enable UART */
#define UART_CR_SIREN           (1U << 1)
#define UART_CR_SIRLP           (1U << 2)
#define UART_CR_LBE             (1U << 7) /* Loopback Enable */
#define UART_CR_TXE             (1U << 8) /* Transmit Enable */
#define UART_CR_RXE             (1U << 9) /* Receive Enable */

/* UART_LCRH Line Control Bits */
#define UART_LCRH_BRK           (1U << 0)
#define UART_LCRH_PEN           (1U << 1) /* Parity Enable */
#define UART_LCRH_EPS           (1U << 2) /* Even Parity Select */
#define UART_LCRH_STP2          (1U << 3) /* 2 Stop Bits */
#define UART_LCRH_FEN           (1U << 4) /* FIFO Enable */
#define UART_LCRH_WLEN_8        (3U << 5) /* 8-bit word length */

/* ========================================================================== */
/* Timer Register Map (CMSDK / Periodic Core Timer)                          */
/* ========================================================================== */
#define TIMER_CTRL_OFFSET       0x00U /* Control Register */
#define TIMER_VALUE_OFFSET      0x04U /* Current Value */
#define TIMER_RELOAD_OFFSET     0x08U /* Reload Value */
#define TIMER_INTSTATUS_OFFSET  0x0CU /* Interrupt Status */
#define TIMER_INTCLEAR_OFFSET   0x0CU /* Interrupt Clear */

#define TIMER_CTRL_EN           (1U << 0)
#define TIMER_CTRL_EXTIN_EN     (1U << 1)
#define TIMER_CTRL_EXTIN_CLK    (1U << 2)
#define TIMER_CTRL_INT_EN       (1U << 3)

/* ========================================================================== */
/* GPIO Register Map                                                          */
/* ========================================================================== */
#define GPIO_MODER_OFFSET       0x00U /* Pin Mode: Input/Output/Alt/Analog */
#define GPIO_OTYPER_OFFSET      0x04U /* Output Type: Push-Pull / Open-Drain */
#define GPIO_OSPEEDR_OFFSET     0x08U /* Output Speed */
#define GPIO_PUPDR_OFFSET       0x0CU /* Pull-Up / Pull-Down */
#define GPIO_IDR_OFFSET         0x10U /* Input Data Register */
#define GPIO_ODR_OFFSET         0x14U /* Output Data Register */
#define GPIO_BSRR_OFFSET        0x18U /* Bit Set/Reset Register */
#define GPIO_LCKR_OFFSET        0x1CU /* Configuration Lock Register */
#define GPIO_AFRL_OFFSET        0x20U /* Alternate Function Low */
#define GPIO_AFRH_OFFSET        0x24U /* Alternate Function High */

/* ========================================================================== */
/* SPI Register Map                                                           */
/* ========================================================================== */
#define SPI_CR1_OFFSET          0x00U /* Control Register 1 */
#define SPI_CR2_OFFSET          0x04U /* Control Register 2 */
#define SPI_SR_OFFSET           0x08U /* Status Register */
#define SPI_DR_OFFSET           0x0CU /* Data Register */

#define SPI_CR1_CPHA            (1U << 0)
#define SPI_CR1_CPOL            (1U << 1)
#define SPI_CR1_MSTR            (1U << 2) /* Master Configuration */
#define SPI_CR1_BR_DIV2         (0U << 3)
#define SPI_CR1_BR_DIV4         (1U << 3)
#define SPI_CR1_BR_DIV8         (2U << 3)
#define SPI_CR1_SPE             (1U << 6) /* SPI Enable */
#define SPI_CR1_LSBFIRST        (1U << 7)
#define SPI_CR1_SSI             (1U << 8)
#define SPI_CR1_SSM             (1U << 9)
#define SPI_CR1_RXONLY          (1U << 10)
#define SPI_CR1_DFF             (1U << 11)
#define SPI_CR1_BIDIOE          (1U << 14)
#define SPI_CR1_BIDIMODE        (1U << 15)

#define SPI_SR_RXNE             (1U << 0) /* RX Buffer Not Empty */
#define SPI_SR_TXE              (1U << 1) /* TX Buffer Empty */
#define SPI_SR_CHSIDE           (1U << 2)
#define SPI_SR_UDR              (1U << 3)
#define SPI_SR_CRCERR           (1U << 4)
#define SPI_SR_MODF             (1U << 5)
#define SPI_SR_OVR              (1U << 6)
#define SPI_SR_BSY              (1U << 7) /* Busy Flag */

/* ========================================================================== */
/* I2C Register Map                                                           */
/* ========================================================================== */
#define I2C_CR1_OFFSET          0x00U /* Control Register 1 */
#define I2C_CR2_OFFSET          0x04U /* Control Register 2 */
#define I2C_OAR1_OFFSET         0x08U /* Own Address Register 1 */
#define I2C_OAR2_OFFSET         0x0CU /* Own Address Register 2 */
#define I2C_DR_OFFSET           0x10U /* Data Register */
#define I2C_SR1_OFFSET          0x14U /* Status Register 1 */
#define I2C_SR2_OFFSET          0x18U /* Status Register 2 */
#define I2C_CCR_OFFSET          0x1CU /* Clock Control Register */
#define I2C_TRISE_OFFSET        0x20U /* Rise Time Register */

#define I2C_CR1_PE              (1U << 0)  /* Peripheral Enable */
#define I2C_CR1_START           (1U << 8)  /* Generate Start */
#define I2C_CR1_STOP            (1U << 9)  /* Generate Stop */
#define I2C_CR1_ACK             (1U << 10) /* Acknowledge Enable */
#define I2C_CR1_SWRST           (1U << 15) /* Software Reset */

#define I2C_SR1_SB              (1U << 0)  /* Start Bit Sent */
#define I2C_SR1_ADDR            (1U << 1)  /* Address Sent / Matched */
#define I2C_SR1_BTF             (1U << 2)  /* Byte Transfer Finished */
#define I2C_SR1_RXNE            (1U << 6)  /* RX Buffer Not Empty */
#define I2C_SR1_TXE             (1U << 7)  /* TX Buffer Empty */
#define I2C_SR1_BERR            (1U << 8)  /* Bus Error */
#define I2C_SR1_ARLO            (1U << 9)  /* Arbitration Lost */
#define I2C_SR1_AF              (1U << 10) /* Acknowledge Failure */
#define I2C_SR1_TIMEOUT         (1U << 14)

#define I2C_SR2_MSL             (1U << 0)  /* Master / Slave mode */
#define I2C_SR2_BUSY            (1U << 1)  /* Bus Busy */

/* ========================================================================== */
/* Watchdog Timer (WDT) Register Map                                         */
/* ========================================================================== */
#define WDT_LOAD_OFFSET         0x00U /* Watchdog Load Register */
#define WDT_VALUE_OFFSET        0x04U /* Watchdog Current Value */
#define WDT_CTRL_OFFSET         0x08U /* Control Register */
#define WDT_INTCLR_OFFSET       0x0CU /* Interrupt / Counter Clear */
#define WDT_RIS_OFFSET          0x10U /* Raw Interrupt Status */
#define WDT_MIS_OFFSET          0x14U /* Masked Interrupt Status */
#define WDT_LOCK_OFFSET         0xC00U/* Watchdog Lock Register */

#define WDT_CTRL_INTEN          (1U << 0) /* Enable Interrupt */
#define WDT_CTRL_RESEN          (1U << 1) /* Enable System Reset on Timeout */
#define WDT_UNLOCK_KEY          0x1ACCE551UL /* ARM Standard Lock Key */

/* ========================================================================== */
/* DMA Controller Register Map (Multi-Channel DMA)                            */
/* ========================================================================== */
#define DMA_STATUS_OFFSET       0x00U /* DMA Status Register */
#define DMA_CFG_OFFSET          0x04U /* DMA Configuration Register */
#define DMA_CH0_CTRL_OFFSET     0x10U /* Channel 0 Control */
#define DMA_CH0_SRC_OFFSET      0x14U /* Channel 0 Source Address */
#define DMA_CH0_DST_OFFSET      0x18U /* Channel 0 Destination Address */
#define DMA_CH0_CNT_OFFSET      0x1CU /* Channel 0 Transfer Count (bytes) */
#define DMA_CH0_STATUS_OFFSET   0x20U /* Channel 0 Status */

#define DMA_CTRL_EN             (1U << 0) /* Channel Enable */
#define DMA_CTRL_SRC_INC        (1U << 1) /* Auto-increment Source */
#define DMA_CTRL_DST_INC        (1U << 2) /* Auto-increment Dest */
#define DMA_CTRL_CIRC           (1U << 3) /* Circular Mode */
#define DMA_CTRL_TCIE           (1U << 4) /* Transfer Complete Interrupt */
#define DMA_CTRL_TEIE           (1U << 5) /* Transfer Error Interrupt */
#define DMA_CTRL_WIDTH_32       (2U << 8) /* 32-bit width */

#define DMA_STATUS_BUSY         (1U << 0)
#define DMA_STATUS_COMPLETE     (1U << 1)
#define DMA_STATUS_ERROR        (1U << 2)

#endif /* SOC_REGS_H */

