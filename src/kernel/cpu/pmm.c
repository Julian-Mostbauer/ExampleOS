#include "pmm.h"
#include "memory.h"
#include "io.h"

// Symbol defined in linker.ld at the end of the BSS section
extern uint32_t _kernel_end;

static uint32_t *pmm_bitmap = NULL;
static uint32_t total_memory = 0;
static uint32_t total_blocks = 0;
static uint32_t used_blocks  = 0;

static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint32_t detect_memory(void) {
    // 1. Check for memory above 16MB reported in 64KB blocks (CMOS 0x5B, 0x5C, 0x5D)
    uint32_t high_blocks = (uint32_t)cmos_read(0x5B) |
                          ((uint32_t)cmos_read(0x5C) << 8) |
                          ((uint32_t)cmos_read(0x5D) << 16);
    if (high_blocks > 0) {
        return (16 * 1024 * 1024) + (high_blocks * 65536);
    }

    // 2. Check for extended memory above 1MB reported in 1KB blocks (CMOS 0x30, 0x31)
    uint32_t ext_kb = (uint32_t)cmos_read(0x30) |
                     ((uint32_t)cmos_read(0x31) << 8);
    if (ext_kb > 0) {
        return (1024 * 1024) + (ext_kb * 1024);
    }

    // 3. Fallback default: 32 MB
    return 32 * 1024 * 1024;
}

void pmm_init(void) {
    total_memory = detect_memory();
    total_blocks = total_memory / PAGE_SIZE;

    // Cap maximum blocks to 512 MB (131072 blocks) to bound bitmap size to 16 KB
    if (total_blocks > 131072) {
        total_blocks = 131072;
        total_memory = total_blocks * PAGE_SIZE;
    }

    // Bitmap starts immediately after the kernel image
    pmm_bitmap = (uint32_t *)PAGE_ALIGN_UP((uint32_t)&_kernel_end);
    uint32_t bitmap_size = (total_blocks + 7) / 8;
    uint32_t bitmap_end = (uint32_t)pmm_bitmap + bitmap_size;

    // Initially mark all blocks as used (all bits 1)
    memset(pmm_bitmap, 0xFF, bitmap_size);
    used_blocks = total_blocks;

    // Mark usable memory above 1MB (0x100000) as free
    if (total_memory > 0x100000) {
        pmm_mark_free(0x100000, total_memory - 0x100000);
    }

    // Ensure lower 1MB is marked used (preserves IVT, BDA, bootloader, kernel, bitmap, stack, VGA)
    pmm_mark_used(0x00000000, 0x00100000);

    // If bitmap extends above 1MB (rare), protect it as well
    if (bitmap_end > 0x100000) {
        pmm_mark_used(0x100000, bitmap_end - 0x100000);
    }
}

void pmm_mark_used(uint32_t base_addr, uint32_t size) {
    uint32_t start_block = PAGE_ALIGN_DOWN(base_addr) / PAGE_SIZE;
    uint32_t end_block = PAGE_ALIGN_UP(base_addr + size) / PAGE_SIZE;
    if (end_block > total_blocks) end_block = total_blocks;

    for (uint32_t b = start_block; b < end_block; b++) {
        if (!(pmm_bitmap[b / 32] & (1U << (b % 32)))) {
            pmm_bitmap[b / 32] |= (1U << (b % 32));
            used_blocks++;
        }
    }
}

void pmm_mark_free(uint32_t base_addr, uint32_t size) {
    uint32_t start_block = PAGE_ALIGN_UP(base_addr) / PAGE_SIZE;
    uint32_t end_block = PAGE_ALIGN_DOWN(base_addr + size) / PAGE_SIZE;
    if (end_block > total_blocks) end_block = total_blocks;

    for (uint32_t b = start_block; b < end_block; b++) {
        if (pmm_bitmap[b / 32] & (1U << (b % 32))) {
            pmm_bitmap[b / 32] &= ~(1U << (b % 32));
            used_blocks--;
        }
    }
}

void *pmm_alloc_block(void) {
    uint32_t num_dwords = total_blocks / 32;

    for (uint32_t i = 0; i < num_dwords; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (int b = 0; b < 32; b++) {
                if (!(pmm_bitmap[i] & (1U << b))) {
                    pmm_bitmap[i] |= (1U << b);
                    used_blocks++;
                    uint32_t block = (i * 32) + (uint32_t)b;
                    return (void *)(block * PAGE_SIZE);
                }
            }
        }
    }

    return NULL; // Out of physical memory
}

void pmm_free_block(void *ptr) {
    if (!ptr) return;

    uint32_t block = (uint32_t)ptr / PAGE_SIZE;
    if (block >= total_blocks) return;

    if (pmm_bitmap[block / 32] & (1U << (block % 32))) {
        pmm_bitmap[block / 32] &= ~(1U << (block % 32));
        used_blocks--;
    }
}

uint32_t pmm_get_total_memory(void) {
    return total_memory;
}

uint32_t pmm_get_total_blocks(void) {
    return total_blocks;
}

uint32_t pmm_get_used_blocks(void) {
    return used_blocks;
}

uint32_t pmm_get_free_blocks(void) {
    return (total_blocks > used_blocks) ? (total_blocks - used_blocks) : 0;
}
