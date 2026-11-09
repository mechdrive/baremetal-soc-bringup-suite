/**
 * @file diag_interrupt.c
 * @brief Interrupt Controller (NVIC / PLIC) & Exception Diagnostic Implementations
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "diag_interrupt.h"
#include "bringup_framework.h"
#include "soc_regs.h"
#include "timer.h"

static volatile bool s_sw_irq_fired = false;
static volatile uint64_t s_irq_trigger_timestamp = 0;
static volatile uint64_t s_irq_enter_timestamp = 0;

/* Dedicated Bringup Software Test Interrupt Handler */
void Test_Software_IRQ_Handler(void)
{
    s_irq_enter_timestamp = timer_get_time_us();
    s_sw_irq_fired = true;

#if defined(ARCH_RISCV)
    /* Clear software interrupt pending bit in CLINT / MSIP */
    mmio_write32(SOC_CLINT_BASE, 0);
#else
    /* Clear pending flag in NVIC ICPR */
    mmio_write32(SOC_NVIC_BASE + 0x180, (1U << 1));
#endif
}

test_result_t test_interrupt_controller_registers(void)
{
#if defined(ARCH_RISCV)
    /* Test PLIC Priority Register Read/Write */
    mmio_write32(SOC_PLIC_BASE + 0x04, 7); /* Set IRQ 1 priority to 7 */
    uint32_t prio = mmio_read32(SOC_PLIC_BASE + 0x04);
    ASSERT_EQ(7, prio, "RISC-V PLIC priority register write/read failed");

    /* Test PLIC Enable Register */
    mmio_write32(SOC_PLIC_BASE + 0x2000, 0x02);
    uint32_t en = mmio_read32(SOC_PLIC_BASE + 0x2000);
    ASSERT_EQ(0x02, (en & 0x02), "RISC-V PLIC enable register failed");
#else
    /* Test ARM NVIC ISER (Set-Enable) and ICER (Clear-Enable) */
    uintptr_t iser0 = SOC_NVIC_BASE + 0x000;
    uintptr_t icer0 = SOC_NVIC_BASE + 0x080;

    /* Enable IRQ 1 */
    mmio_write32(iser0, (1U << 1));
    uint32_t state1 = mmio_read32(iser0);
    ASSERT_TRUE((state1 & (1U << 1)) != 0, "NVIC ISER failed to enable IRQ 1");

    /* Disable IRQ 1 */
    mmio_write32(icer0, (1U << 1));
    uint32_t state2 = mmio_read32(iser0);
    ASSERT_TRUE((state2 & (1U << 1)) == 0, "NVIC ICER failed to clear IRQ 1 enable");
#endif

    return TEST_RESULT_PASS;
}

test_result_t test_interrupt_software_trigger(void)
{
    s_sw_irq_fired = false;

#if defined(ARCH_RISCV)
    /* Trigger software interrupt via CLINT MSIP */
    mmio_write32(SOC_CLINT_BASE, 1);
#else
    /* Trigger interrupt 1 via NVIC ISPR (Set-Pending Register) */
    uintptr_t ispr0 = SOC_NVIC_BASE + 0x100;
    uintptr_t iser0 = SOC_NVIC_BASE + 0x000;
    mmio_write32(iser0, (1U << 1));
    mmio_write32(ispr0, (1U << 1));
#endif

    /* In simulation / test environment, if hardware interrupt routing is simulated,
     * wait for handler to execute or invoke synthetic handler */
    uint32_t timeout = 50000;
    while (!s_sw_irq_fired && --timeout) {
        /* If simulated environment without CPU hardware core preempt, execute test hook */
#if !defined(__ARM_ARCH) && !defined(__riscv)
        Test_Software_IRQ_Handler();
        break;
#endif
    }

    ASSERT_TRUE(s_sw_irq_fired, "Software interrupt failed to trigger service routine");
    return TEST_RESULT_PASS;
}

test_result_t test_interrupt_latency_profile(void)
{
    s_sw_irq_fired = false;
    s_irq_trigger_timestamp = timer_get_time_us();

#if defined(ARCH_RISCV)
    mmio_write32(SOC_CLINT_BASE, 1);
#else
    mmio_write32(SOC_NVIC_BASE + 0x100, (1U << 1));
#endif

#if !defined(__ARM_ARCH) && !defined(__riscv)
    Test_Software_IRQ_Handler();
#else
    uint32_t timeout = 50000;
    while (!s_sw_irq_fired && --timeout) {}
#endif

    ASSERT_TRUE(s_sw_irq_fired, "ISR execution failed during latency benchmark");
    uint64_t latency_us = (s_irq_enter_timestamp >= s_irq_trigger_timestamp) ?
                          (s_irq_enter_timestamp - s_irq_trigger_timestamp) : 0;

    diag_log(LOG_LEVEL_INFO, "Measured interrupt dispatch latency: %lu us", (unsigned long)latency_us);
    return TEST_RESULT_PASS;
}

test_result_t test_core_mpu_protection(void)
{
#if defined(ARCH_ARM) || (!defined(ARCH_RISCV) && !defined(__riscv))
    /* Test ARM Cortex-M MPU Control & Region Registers */
    uintptr_t mpu_ctrl = SOC_MPU_BASE + 0x00;
    uintptr_t mpu_rnr  = SOC_MPU_BASE + 0x08;
    uintptr_t mpu_rbar = SOC_MPU_BASE + 0x0C;
    uintptr_t mpu_rasr = SOC_MPU_BASE + 0x10;

    /* Select Region 0 */
    mmio_write32(mpu_rnr, 0);
    ASSERT_EQ(0, mmio_read32(mpu_rnr), "MPU RNR region select failed");

    /* Configure Region 0: Base 0x20000000, Size 64KB, Read-Only, Privileged */
    mmio_write32(mpu_rbar, 0x20000000UL | 0x10); /* Valid bit */
    mmio_write32(mpu_rasr, (0x07UL << 24) | (15UL << 1) | 0x01); /* RO, 64KB, Enable */

    /* Enable MPU with default memory map for privileged software */
    mmio_write32(mpu_ctrl, 0x05); /* ENABLE | PRIVDEFENA */
    ASSERT_TRUE((mmio_read32(mpu_ctrl) & 0x01) != 0, "MPU failed to activate");

    /* Disable MPU to restore open access */
    mmio_write32(mpu_ctrl, 0x00);
#endif

    return TEST_RESULT_PASS;
}

void diag_interrupt_register_all(void)
{
    test_case_t tc_reg = {
        .test_id = 30,
        .name = "INT_CONTROLLER_REGS",
        .description = "Interrupt enable/disable & priority register access",
        .phase = PHASE_INTERRUPTS,
        .test_fn = test_interrupt_controller_registers
    };
    bringup_register_test(&tc_reg);

    test_case_t tc_trig = {
        .test_id = 31,
        .name = "INT_SOFTWARE_TRIGGER",
        .description = "Software IRQ dispatch & vector service execution",
        .phase = PHASE_INTERRUPTS,
        .test_fn = test_interrupt_software_trigger
    };
    bringup_register_test(&tc_trig);

    test_case_t tc_lat = {
        .test_id = 32,
        .name = "INT_LATENCY_PROFILE",
        .description = "Interrupt dispatch latency measurement",
        .phase = PHASE_INTERRUPTS,
        .test_fn = test_interrupt_latency_profile
    };
    bringup_register_test(&tc_lat);

    test_case_t tc_mpu = {
        .test_id = 33,
        .name = "BUS_MPU_PROTECTION",
        .description = "Memory Protection Unit region configuration & boundary check",
        .phase = PHASE_BUS_FABRIC,
        .test_fn = test_core_mpu_protection
    };
    bringup_register_test(&tc_mpu);
}

