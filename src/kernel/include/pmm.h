#ifndef _PMM_H_
#define _PMM_H_

#include "types.h"

#define PAGE_SIZE 4096
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)   (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

void pmm_init(void);
void *pmm_alloc_block(void);
void pmm_free_block(void *ptr);
void pmm_mark_used(uint32_t base_addr, uint32_t size);
void pmm_mark_free(uint32_t base_addr, uint32_t size);

uint32_t pmm_get_total_memory(void);
uint32_t pmm_get_total_blocks(void);
uint32_t pmm_get_used_blocks(void);
uint32_t pmm_get_free_blocks(void);

#endif
