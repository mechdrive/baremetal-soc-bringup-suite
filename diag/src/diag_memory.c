/**
 * @file diag_memory.c
 * @brief Memory Subsystem Diagnostic Test Implementations
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "diag_memory.h"
#include "bringup_framework.h"
#include "soc_regs.h"

/* Diagnostic buffer placed in safe SRAM region */
#define TEST_RAM_OFFSET     (64 * 1024UL) /* 64KB above stack/data */
#define TEST_RAM_SIZE_WORDS 1024UL        /* 4KB test block */

static uintptr_t get_test_ram_base(void)
{
    return SOC_RAM_BASE + TEST_RAM_OFFSET;
}

test_result_t diag_mem_walking_ones(uintptr_t base_addr)
{
    volatile uint32_t *target = (volatile uint32_t *)base_addr;

    for (int bit = 0; bit < 32; ++bit) {
        uint32_t pattern = (1UL << bit);
        *target = pattern;
        MMIO_BARRIER();
        uint32_t readback = *target;

        if (readback != pattern) {
            if (g_current_test) {
                g_current_test->fail_address = base_addr;
                g_current_test->expected_val = pattern;
                g_current_test->actual_val = readback;
                g_current_test->fail_msg = "Data bus bit stuck or shorted (Walking 1)";
            }
            return TEST_RESULT_FAIL;
        }
    }
    return TEST_RESULT_PASS;
}

test_result_t diag_mem_walking_zeros(uintptr_t base_addr)
{
    volatile uint32_t *target = (volatile uint32_t *)base_addr;

    for (int bit = 0; bit < 32; ++bit) {
        uint32_t pattern = ~(1UL << bit);
        *target = pattern;
        MMIO_BARRIER();
        uint32_t readback = *target;

        if (readback != pattern) {
            if (g_current_test) {
                g_current_test->fail_address = base_addr;
                g_current_test->expected_val = pattern;
                g_current_test->actual_val = readback;
                g_current_test->fail_msg = "Data bus bit stuck-at-1 (Walking 0)";
            }
            return TEST_RESULT_FAIL;
        }
    }
    return TEST_RESULT_PASS;
}

test_result_t diag_mem_address_bus_integrity(uintptr_t base_addr, size_t size_bytes)
{
    volatile uint32_t *base = (volatile uint32_t *)base_addr;
    size_t num_words = size_bytes / sizeof(uint32_t);

    /* 1. Write unique pattern to each power-of-two offset */
    for (size_t offset = 1; offset < num_words; offset <<= 1) {
        base[offset] = (uint32_t)(0xAA550000UL | offset);
    }

    /* 2. Write inverted pattern to base */
    base[0] = 0x55AAFFFFUL;
    MMIO_BARRIER();

    /* 3. Check for aliasing: verify base value didn't overwrite power-of-two lines */
    for (size_t offset = 1; offset < num_words; offset <<= 1) {
        uint32_t expected = (uint32_t)(0xAA550000UL | offset);
        uint32_t actual = base[offset];
        if (actual != expected) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&base[offset];
                g_current_test->expected_val = expected;
                g_current_test->actual_val = actual;
                g_current_test->fail_msg = "Address line aliasing or short detected";
            }
            return TEST_RESULT_FAIL;
        }
    }

    return TEST_RESULT_PASS;
}

test_result_t diag_mem_march_c_minus(uintptr_t base_addr, size_t num_words)
{
    volatile uint32_t *mem = (volatile uint32_t *)base_addr;

    /* Element 1: ⇕ (w0) */
    for (size_t i = 0; i < num_words; ++i) {
        mem[i] = 0x00000000UL;
    }
    MMIO_BARRIER();

    /* Element 2: ⇑ (r0, w1) */
    for (size_t i = 0; i < num_words; ++i) {
        uint32_t val = mem[i];
        if (val != 0x00000000UL) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&mem[i];
                g_current_test->expected_val = 0x00000000UL;
                g_current_test->actual_val = val;
                g_current_test->fail_msg = "March C- Step 2 (r0) failed";
            }
            return TEST_RESULT_FAIL;
        }
        mem[i] = 0xFFFFFFFFUL;
    }
    MMIO_BARRIER();

    /* Element 3: ⇑ (r1, w0) */
    for (size_t i = 0; i < num_words; ++i) {
        uint32_t val = mem[i];
        if (val != 0xFFFFFFFFUL) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&mem[i];
                g_current_test->expected_val = 0xFFFFFFFFUL;
                g_current_test->actual_val = val;
                g_current_test->fail_msg = "March C- Step 3 (r1) failed";
            }
            return TEST_RESULT_FAIL;
        }
        mem[i] = 0x00000000UL;
    }
    MMIO_BARRIER();

    /* Element 4: ⇓ (r0, w1) */
    for (size_t i = num_words; i > 0; --i) {
        size_t idx = i - 1;
        uint32_t val = mem[idx];
        if (val != 0x00000000UL) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&mem[idx];
                g_current_test->expected_val = 0x00000000UL;
                g_current_test->actual_val = val;
                g_current_test->fail_msg = "March C- Step 4 (r0) failed";
            }
            return TEST_RESULT_FAIL;
        }
        mem[idx] = 0xFFFFFFFFUL;
    }
    MMIO_BARRIER();

    /* Element 5: ⇓ (r1, w0) */
    for (size_t i = num_words; i > 0; --i) {
        size_t idx = i - 1;
        uint32_t val = mem[idx];
        if (val != 0xFFFFFFFFUL) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&mem[idx];
                g_current_test->expected_val = 0xFFFFFFFFUL;
                g_current_test->actual_val = val;
                g_current_test->fail_msg = "March C- Step 5 (r1) failed";
            }
            return TEST_RESULT_FAIL;
        }
        mem[idx] = 0x00000000UL;
    }
    MMIO_BARRIER();

    /* Element 6: ⇕ (r0) */
    for (size_t i = 0; i < num_words; ++i) {
        uint32_t val = mem[i];
        if (val != 0x00000000UL) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&mem[i];
                g_current_test->expected_val = 0x00000000UL;
                g_current_test->actual_val = val;
                g_current_test->fail_msg = "March C- Step 6 final (r0) failed";
            }
            return TEST_RESULT_FAIL;
        }
    }

    return TEST_RESULT_PASS;
}

test_result_t diag_mem_lfsr_pattern(uintptr_t base_addr, size_t num_words, uint32_t seed)
{
    volatile uint32_t *mem = (volatile uint32_t *)base_addr;
    uint32_t lfsr = (seed == 0) ? 0xACE1ACE1UL : seed;

    /* Write Galois LFSR pseudo-random sequence */
    for (size_t i = 0; i < num_words; ++i) {
        mem[i] = lfsr;
        /* Polynomial: x^32 + x^31 + x^29 + x + 1 */
        lfsr = (lfsr >> 1) ^ (-(int32_t)(lfsr & 1) & 0xD0000001UL);
    }
    MMIO_BARRIER();

    /* Reset LFSR to initial seed and verify readback */
    lfsr = (seed == 0) ? 0xACE1ACE1UL : seed;
    for (size_t i = 0; i < num_words; ++i) {
        uint32_t expected = lfsr;
        uint32_t actual = mem[i];
        if (actual != expected) {
            if (g_current_test) {
                g_current_test->fail_address = (uintptr_t)&mem[i];
                g_current_test->expected_val = expected;
                g_current_test->actual_val = actual;
                g_current_test->fail_msg = "LFSR pseudo-random pattern mismatch";
            }
            return TEST_RESULT_FAIL;
        }
        lfsr = (lfsr >> 1) ^ (-(int32_t)(lfsr & 1) & 0xD0000001UL);
    }

    return TEST_RESULT_PASS;
}

/* -------------------------------------------------------------------------- */
/* Framework Test Wrappers                                                    */
/* -------------------------------------------------------------------------- */

test_result_t test_mem_walking_ones(void)
{
    return diag_mem_walking_ones(get_test_ram_base());
}

test_result_t test_mem_walking_zeros(void)
{
    return diag_mem_walking_zeros(get_test_ram_base());
}

test_result_t test_mem_address_bus_integrity(void)
{
    return diag_mem_address_bus_integrity(get_test_ram_base(), TEST_RAM_SIZE_WORDS * sizeof(uint32_t));
}

test_result_t test_mem_march_c_minus(void)
{
    return diag_mem_march_c_minus(get_test_ram_base(), TEST_RAM_SIZE_WORDS);
}

test_result_t test_mem_lfsr_pattern(void)
{
    return diag_mem_lfsr_pattern(get_test_ram_base(), TEST_RAM_SIZE_WORDS, 0x12345678UL);
}

void diag_memory_register_all(void)
{
    test_case_t tc1 = {
        .test_id = 10,
        .name = "MEM_WALK_ONES",
        .description = "Data bus walking 1s stuck-at-0 verification",
        .phase = PHASE_MEMORY_SRAM,
        .test_fn = test_mem_walking_ones
    };
    bringup_register_test(&tc1);

    test_case_t tc2 = {
        .test_id = 11,
        .name = "MEM_WALK_ZEROS",
        .description = "Data bus walking 0s stuck-at-1 verification",
        .phase = PHASE_MEMORY_SRAM,
        .test_fn = test_mem_walking_zeros
    };
    bringup_register_test(&tc2);

    test_case_t tc3 = {
        .test_id = 12,
        .name = "MEM_ADDR_BUS_ALIAS",
        .description = "Address line aliasing & bridge isolation",
        .phase = PHASE_MEMORY_SRAM,
        .test_fn = test_mem_address_bus_integrity
    };
    bringup_register_test(&tc3);

    test_case_t tc4 = {
        .test_id = 13,
        .name = "MEM_MARCH_C_MINUS",
        .description = "6-pass March C- silicon SRAM cell qualification",
        .phase = PHASE_MEMORY_SRAM,
        .test_fn = test_mem_march_c_minus
    };
    bringup_register_test(&tc4);

    test_case_t tc5 = {
        .test_id = 14,
        .name = "MEM_LFSR_STRESS",
        .description = "Galois LFSR pseudo-random bit retention test",
        .phase = PHASE_MEMORY_SRAM,
        .test_fn = test_mem_lfsr_pattern
    };
    bringup_register_test(&tc5);
}
