#ifndef _SEC_MEMINFO_H_
#define _SEC_MEMINFO_H_
void sec_meminfo_set_alloc_cnt(int alloc_type, int is_alloc, struct page *page);
void sec_meminfo_print(void);
#endif
