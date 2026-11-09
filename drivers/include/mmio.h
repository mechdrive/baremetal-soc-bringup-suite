/**
 * @file mmio.h
 * @brief Memory-Mapped I/O (MMIO) Access Primitives & Memory Barriers
 *
 * Provides volatile-qualified, barrier-enforced 8-bit, 16-bit, and 32-bit register
 * access primitives for bare-metal ARM Cortex-M, RISC-V, and Virtual SoC Simulation.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Architecture-specific barrier primitives */
#if defined(__ARM_ARCH) || defined(__arm__) || defined(__thumb__)
    #define MMIO_BARRIER()      __asm__ volatile ("dmb 0xF" ::: "memory")
    #define MMIO_INST_BARRIER() __asm__ volatile ("isb 0xF" ::: "memory")
#elif defined(__riscv) || defined(__riscv__)
    #define MMIO_BARRIER()      __asm__ volatile ("fence iorw, iorw" ::: "memory")
    #define MMIO_INST_BARRIER() __asm__ volatile ("fence.i" ::: "memory")
#else
    #define MMIO_BARRIER()      do {} while(0)
    #define MMIO_INST_BARRIER() do {} while(0)
#endif

#if defined(SIMULATION)

/* Simulated bus prototypes */
uint32_t sim_mmio_read32(uintptr_t addr);
void     sim_mmio_write32(uintptr_t addr, uint32_t val);
uint16_t sim_mmio_read16(uintptr_t addr);
void     sim_mmio_write16(uintptr_t addr, uint16_t val);
uint8_t  sim_mmio_read8(uintptr_t addr);
void     sim_mmio_write8(uintptr_t addr, uint8_t val);

static inline uint32_t mmio_read32(uintptr_t addr)
{
    return sim_mmio_read32(addr);
}

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    sim_mmio_write32(addr, val);
}

static inline uint16_t mmio_read16(uintptr_t addr)
{
    return sim_mmio_read16(addr);
}

static inline void mmio_write16(uintptr_t addr, uint16_t val)
{
    sim_mmio_write16(addr, val);
}

static inline uint8_t mmio_read8(uintptr_t addr)
{
    return sim_mmio_read8(addr);
}

static inline void mmio_write8(uintptr_t addr, uint8_t val)
{
    sim_mmio_write8(addr, val);
}

#else /* Bare-metal direct hardware memory-mapped access */

static inline uint32_t mmio_read32(uintptr_t addr)
{
    volatile uint32_t *reg = (volatile uint32_t *)addr;
    uint32_t val = *reg;
    MMIO_BARRIER();
    return val;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    MMIO_BARRIER();
    volatile uint32_t *reg = (volatile uint32_t *)addr;
    *reg = val;
    MMIO_BARRIER();
}

static inline uint16_t mmio_read16(uintptr_t addr)
{
    volatile uint16_t *reg = (volatile uint16_t *)addr;
    uint16_t val = *reg;
    MMIO_BARRIER();
    return val;
}

static inline void mmio_write16(uintptr_t addr, uint16_t val)
{
    MMIO_BARRIER();
    volatile uint16_t *reg = (volatile uint16_t *)addr;
    *reg = val;
    MMIO_BARRIER();
}

static inline uint8_t mmio_read8(uintptr_t addr)
{
    volatile uint8_t *reg = (volatile uint8_t *)addr;
    uint8_t val = *reg;
    MMIO_BARRIER();
    return val;
}

static inline void mmio_write8(uintptr_t addr, uint8_t val)
{
    MMIO_BARRIER();
    volatile uint8_t *reg = (volatile uint8_t *)addr;
    *reg = val;
    MMIO_BARRIER();
}

#endif /* SIMULATION */

static inline void mmio_set_bits32(uintptr_t addr, uint32_t mask)
{
    uint32_t val = mmio_read32(addr);
    mmio_write32(addr, val | mask);
}

static inline void mmio_clear_bits32(uintptr_t addr, uint32_t mask)
{
    uint32_t val = mmio_read32(addr);
    mmio_write32(addr, val & ~mask);
}

static inline void mmio_toggle_bits32(uintptr_t addr, uint32_t mask)
{
    uint32_t val = mmio_read32(addr);
    mmio_write32(addr, val ^ mask);
}

static inline void mmio_modify32(uintptr_t addr, uint32_t clear_mask, uint32_t set_mask)
{
    uint32_t val = mmio_read32(addr);
    val = (val & ~clear_mask) | set_mask;
    mmio_write32(addr, val);
}

static inline bool mmio_poll_bits_set(uintptr_t addr, uint32_t mask, uint32_t timeout_loops)
{
    uint32_t count = 0;
    while ((mmio_read32(addr) & mask) != mask) {
        if (timeout_loops > 0 && ++count >= timeout_loops) {
            return false;
        }
    }
    return true;
}

static inline bool mmio_poll_bits_cleared(uintptr_t addr, uint32_t mask, uint32_t timeout_loops)
{
    uint32_t count = 0;
    while ((mmio_read32(addr) & mask) != 0U) {
        if (timeout_loops > 0 && ++count >= timeout_loops) {
            return false;
        }
    }
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* MMIO_H */
