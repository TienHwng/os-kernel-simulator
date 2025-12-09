/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "mm64.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid) {
    struct vm_area_struct *pvma = mm->mmap;

    // caller -> krnl -> mm -> mmap;

    if (mm->mmap == NULL)
        return NULL;

    int vmait = pvma->vm_id;

    while (vmait < vmaid) {
        if (pvma == NULL)
            return NULL;

        pvma = pvma->vm_next;
        vmait = pvma->vm_id;
    }

    return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn, addr_t swpfpn) {
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz) {
    
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
    
    /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
	//struct vm_area_struct *cur_vma = get_vma_by_num(caller->kernl->mm, vmaid);

	//newrg = malloc(sizeof(struct vm_rg_struct));

	/* TODO: update the newrg boundary
	// newrg->rg_start = ...
	// newrg->rg_end = ...
	*/

    if (cur_vma == NULL)
        return NULL;
    
    struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));

    newrg->rg_start = cur_vma->sbrk;
    newrg->rg_end = newrg->rg_start + size;
    newrg->rg_next = NULL;

    return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend) {
    if (vmastart >= vmaend) {
        return -1;
    }

    struct vm_area_struct *vma = caller->krnl->mm->mmap;
    if (vma == NULL) {
        return -1;
    }
    
    while (vma != NULL) {
        if (vma->vm_id != vmaid && OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end)) {
            return -1;
        }
        vma = vma->vm_next;
    }

    return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz) {
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
    if (cur_vma == NULL)
        return -1;
    
    int aligned = PAGING_PAGE_ALIGNSZ(inc_sz);
    int npages = aligned / PAGING_PAGESZ;
    
    struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, aligned);
    if (area == NULL)
        return -1;
    
    if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
        free(area);
        return -1; // Overlap detected
    }
    
    int old_end = cur_vma->vm_end;
    cur_vma->vm_end += inc_sz;
    cur_vma->sbrk += inc_sz;
    
    struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
    if (newrg == NULL) {
        free(area);
        return -1;
    }
    
    if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, npages, newrg) < 0) {
        free(area);
        free(newrg);
        return -1; // Mapping failed
    }
    
    free(area);
    enlist_vm_rg_node(&cur_vma->vm_freerg_list, newrg);
    return 0;
}

// #endif