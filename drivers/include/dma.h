/**
 * @file dma.h
 * @brief Direct Memory Access (DMA) Controller Driver
 *
 * Provides high-throughput memory-to-memory and memory-to-peripheral block transfers,
 * burst mode, circular buffer operation, and transfer completion status checking.
 *
 * @author Poonam Singh
 * @date 2026
 */

#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMA_DIR_MEM_TO_MEM = 0,
    DMA_DIR_MEM_TO_PERIPH,
    DMA_DIR_PERIPH_TO_MEM
} dma_direction_t;

typedef enum {
    DMA_WIDTH_8BIT = 0,
    DMA_WIDTH_16BIT,
    DMA_WIDTH_32BIT
} dma_width_t;

typedef struct {
    uint8_t channel;
    dma_direction_t direction;
    dma_width_t width;
    bool src_inc;
    bool dst_inc;
    bool circular;
    bool enable_interrupt;
} dma_channel_config_t;

/**
 * @brief Initialize DMA controller base.
 * @param base_addr MMIO base address
 * @return 0 on success
 */
int dma_init(uintptr_t base_addr);

/**
 * @brief Configure a specific DMA channel.
 * @param base_addr MMIO base address
 * @param config Pointer to channel configuration
 * @return 0 on success, negative error code otherwise
 */
int dma_config_channel(uintptr_t base_addr, const dma_channel_config_t *config);

/**
 * @brief Start a non-blocking DMA transfer.
 * @param base_addr MMIO base address
 * @param channel Channel index
 * @param src Source address in memory or peripheral register
 * @param dst Destination address in memory or peripheral register
 * @param length_bytes Total bytes to transfer
 * @return 0 on success, negative error code otherwise
 */
int dma_start_transfer(uintptr_t base_addr, uint8_t channel, uintptr_t src, uintptr_t dst, size_t length_bytes);

/**
 * @brief Synchronous blocking DMA memory copy.
 * @param base_addr MMIO base address
 * @param channel Channel index
 * @param dst Destination pointer
 * @param src Source pointer
 * @param length_bytes Total bytes to copy
 * @param timeout_loops Loop timeout for completion
 * @return true on transfer complete, false on error/timeout
 */
bool dma_memcpy_sync(uintptr_t base_addr, uint8_t channel, void *dst, const void *src, size_t length_bytes, uint32_t timeout_loops);

/**
 * @brief Check if channel is currently busy transferring data.
 * @param base_addr MMIO base address
 * @param channel Channel index
 * @return true if active, false if idle/finished
 */
bool dma_is_busy(uintptr_t base_addr, uint8_t channel);

/**
 * @brief Stop active channel transfer immediately.
 * @param base_addr MMIO base address
 * @param channel Channel index
 */
void dma_abort(uintptr_t base_addr, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DMA_H */

