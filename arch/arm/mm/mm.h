#ifdef CONFIG_MMU

/* the upper-most page table pointer */
extern pmd_t *top_pmd;

#define TOP_PTE(x)	pte_offset_kernel(top_pmd, x)

static inline pmd_t *pmd_off_k(unsigned long virt)
{
	return pmd_offset(pud_offset(pgd_offset_k(virt), virt), virt);
}

struct mem_type {
	pteval_t prot_pte;
	unsigned int prot_l1;
	unsigned int prot_sect;
	unsigned int domain;
};

const struct mem_type *get_mem_type(unsigned int type);

extern void __flush_dcache_page(struct address_space *mapping, struct page *page);

#endif

#ifdef CONFIG_ZONE_DMA
extern phys_addr_t arm_dma_limit;
#else
#define arm_dma_limit ((u32)~0)
#endif

#ifdef CONFIG_DMA_CMA
extern phys_addr_t arm_lowmem_limit;
#endif

void __init bootmem_init(void);
void arm_mm_memblock_reserve(void);
#ifdef CONFIG_DMA_CMA
void dma_contiguous_remap(void);
#endif

