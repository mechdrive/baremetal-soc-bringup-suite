/**
 * @file diag_stress.c
 * @brief Silicon Stress & Concurrent Bus Traffic Implementations
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "diag_stress.h"
#include "diag_memory.h"
#include "bringup_framework.h"
#include "soc_regs.h"
#include "dma.h"
#include "timer.h"
#include "uart.h"

#define STRESS_BLOCK_WORDS 512

static uint32_t s_dma_src[STRESS_BLOCK_WORDS];
static uint32_t s_dma_dst[STRESS_BLOCK_WORDS];
static uint32_t s_cpu_mem[STRESS_BLOCK_WORDS];

test_result_t test_stress_concurrent_bus_traffic(void)
{
    /* Initialize DMA source memory */
    for (size_t i = 0; i < STRESS_BLOCK_WORDS; ++i) {
        s_dma_src[i] = (uint32_t)(0x11220000UL | i);
        s_dma_dst[i] = 0;
    }

    /* 1. Configure and start background DMA transfer */
    dma_channel_config_t cfg = {
        .channel = 1,
        .direction = DMA_DIR_MEM_TO_MEM,
        .width = DMA_WIDTH_32BIT,
        .src_inc = true,
        .dst_inc = true,
        .circular = false,
        .enable_interrupt = false
    };

    int init_res = dma_config_channel(SOC_DMA0_BASE, &cfg);
    ASSERT_EQ(0, init_res, "DMA stress channel configuration failed");

    int start_res = dma_start_transfer(SOC_DMA0_BASE, 1,
                                       (uintptr_t)s_dma_src,
                                       (uintptr_t)s_dma_dst,
                                       sizeof(s_dma_src));
    ASSERT_EQ(0, start_res, "DMA background transfer failed to start");

    /* 2. Simultaneously execute March C- memory test on separate memory block using CPU */
    test_result_t mem_res = diag_mem_march_c_minus((uintptr_t)s_cpu_mem, STRESS_BLOCK_WORDS);
    ASSERT_EQ(TEST_RESULT_PASS, mem_res, "Memory verification corrupted during concurrent DMA traffic");

    /* 3. Wait for DMA transfer to complete */
    uint32_t timeout = 100000;
    while (dma_is_busy(SOC_DMA0_BASE, 1)) {
        if (--timeout == 0) {
            dma_abort(SOC_DMA0_BASE, 1);
            ASSERT_TRUE(false, "DMA transfer timed out under bus contention");
        }
    }

    /* 4. Verify DMA transfer integrity */
    for (size_t i = 0; i < STRESS_BLOCK_WORDS; ++i) {
        if (s_dma_dst[i] != s_dma_src[i]) {
            ASSERT_EQ(s_dma_src[i], s_dma_dst[i], "DMA data corrupted by bus contention");
        }
    }

    return TEST_RESULT_PASS;
}

test_result_t test_stress_burst_bandwidth(void)
{
    const size_t words = 1024;
    uint32_t block[words];

    uint64_t t_start = timer_get_time_us();

    /* 10 iterations of block fill + verify */
    for (int iter = 0; iter < 10; ++iter) {
        for (size_t i = 0; i < words; ++i) {
            block[i] = (uint32_t)(iter + i);
        }
        MMIO_BARRIER();
        for (size_t i = 0; i < words; ++i) {
            if (block[i] != (uint32_t)(iter + i)) {
                ASSERT_TRUE(false, "Burst write/read validation failed");
            }
        }
    }

    uint64_t t_end = timer_get_time_us();
    uint64_t elapsed_us = t_end - t_start;
    if (elapsed_us == 0) elapsed_us = 1;

    size_t total_bytes = 10 * words * sizeof(uint32_t) * 2; /* write + read */
    uint64_t mb_per_sec = (total_bytes * 1000000ULL) / (elapsed_us * 1024 * 1024);

    diag_log(LOG_LEVEL_INFO, "SRAM Bus Throughput: %lu MB/s (%lu bytes in %lu us)",
             (unsigned long)mb_per_sec, (unsigned long)total_bytes, (unsigned long)elapsed_us);

    return TEST_RESULT_PASS;
}

void diag_stress_register_all(void)
{
    test_case_t tc_bus = {
        .test_id = 40,
        .name = "STRESS_BUS_CONCURRENCY",
        .description = "Concurrent CPU March C- & DMA bus matrix contention",
        .phase = PHASE_STRESS_BURNIN,
        .test_fn = test_stress_concurrent_bus_traffic
    };
    bringup_register_test(&tc_bus);

    test_case_t tc_bw = {
        .test_id = 41,
        .name = "STRESS_BURST_BANDWIDTH",
        .description = "High-density burst memory read/write throughput",
        .phase = PHASE_STRESS_BURNIN,
        .test_fn = test_stress_burst_bandwidth
    };
    bringup_register_test(&tc_bw);
}
