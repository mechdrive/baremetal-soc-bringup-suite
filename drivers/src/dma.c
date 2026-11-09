/**
 * @file dma.c
 * @brief Direct Memory Access (DMA) Driver Implementation
 *
 * @author Poonam Singh
 * @date 2026
 */

#include "dma.h"
#include "mmio.h"
#include "soc_regs.h"

int dma_init(uintptr_t base_addr)
{
    if (base_addr == 0) return -1;
    /* Reset and enable DMA master controller */
    mmio_write32(base_addr + DMA_CFG_OFFSET, 0x01);
    return 0;
}

int dma_config_channel(uintptr_t base_addr, const dma_channel_config_t *config)
{
    if (base_addr == 0 || config == 0) return -1;

    uint32_t ch_offset = DMA_CH0_CTRL_OFFSET + ((uint32_t)config->channel * 0x20);
    uint32_t ctrl = 0;

    if (config->src_inc) ctrl |= DMA_CTRL_SRC_INC;
    if (config->dst_inc) ctrl |= DMA_CTRL_DST_INC;
    if (config->circular) ctrl |= DMA_CTRL_CIRC;
    if (config->enable_interrupt) ctrl |= (DMA_CTRL_TCIE | DMA_CTRL_TEIE);
    if (config->width == DMA_WIDTH_32BIT) ctrl |= DMA_CTRL_WIDTH_32;

    mmio_write32(base_addr + ch_offset, ctrl);
    return 0;
}

int dma_start_transfer(uintptr_t base_addr, uint8_t channel, uintptr_t src, uintptr_t dst, size_t length_bytes)
{
    if (base_addr == 0 || length_bytes == 0) return -1;

    uint32_t ch_base = base_addr + DMA_CH0_CTRL_OFFSET + ((uint32_t)channel * 0x20);

    /* 1. Set source and destination memory addresses */
    mmio_write32(ch_base + 0x04, (uint32_t)src);
    mmio_write32(ch_base + 0x08, (uint32_t)dst);

    /* 2. Set transfer count */
    mmio_write32(ch_base + 0x0C, (uint32_t)length_bytes);

    /* 3. Enable channel */
    mmio_set_bits32(ch_base, DMA_CTRL_EN);

    return 0;
}

bool dma_is_busy(uintptr_t base_addr, uint8_t channel)
{
    if (base_addr == 0) return false;
    uint32_t ch_base = base_addr + DMA_CH0_CTRL_OFFSET + ((uint32_t)channel * 0x20);
    uint32_t status = mmio_read32(ch_base + 0x10);
    return (status & DMA_STATUS_BUSY) != 0;
}

bool dma_memcpy_sync(uintptr_t base_addr, uint8_t channel, void *dst, const void *src, size_t length_bytes, uint32_t timeout_loops)
{
    if (base_addr == 0 || dst == 0 || src == 0 || length_bytes == 0) return false;

    dma_channel_config_t cfg = {
        .channel = channel,
        .direction = DMA_DIR_MEM_TO_MEM,
        .width = DMA_WIDTH_32BIT,
        .src_inc = true,
        .dst_inc = true,
        .circular = false,
        .enable_interrupt = false
    };

    if (dma_config_channel(base_addr, &cfg) != 0) {
        return false;
    }

    if (dma_start_transfer(base_addr, channel, (uintptr_t)src, (uintptr_t)dst, length_bytes) != 0) {
        return false;
    }

    uint32_t loops = 0;
    while (dma_is_busy(base_addr, channel)) {
        if (timeout_loops > 0 && ++loops >= timeout_loops) {
            dma_abort(base_addr, channel);
            return false;
        }
    }

    uint32_t ch_base = base_addr + DMA_CH0_CTRL_OFFSET + ((uint32_t)channel * 0x20);
    uint32_t status = mmio_read32(ch_base + 0x10);
    return (status & DMA_STATUS_COMPLETE) != 0;
}

void dma_abort(uintptr_t base_addr, uint8_t channel)
{
    if (base_addr == 0) return;
    uint32_t ch_base = base_addr + DMA_CH0_CTRL_OFFSET + ((uint32_t)channel * 0x20);
    mmio_clear_bits32(ch_base, DMA_CTRL_EN);
}
