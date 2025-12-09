/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
// #include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
	struct vm_area_struct *pvma = mm->mmap; // list of vm_area_struct
//caller -> krnl -> mm -> mmap;
	if (mm->mmap == NULL)
		return NULL;

	int vmait = pvma->vm_id; // current vma id 
	while(pvma != NULL){
		if(pvma->vm_id == vmaid) return pvma;
		pvma = pvma->vm_next;
	}
	

	return NULL; // get access to the requested vma base on vmaid
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
	__swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
	return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma start
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
	struct vm_rg_struct * newrg;
	/* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
	// sbrk : system break
	// newrg = malloc(sizeof(struct vm_rg_struct));
	// newrg->rg_start = cur_vma->sbrk;
	// newrg->rg_end = newrg->rg_start + size;
	
	struct vm_area_struct *current_vma = get_vma_by_num(caller->krnl->mm, vmaid);
	if( current_vma == NULL) return NULL;

	newrg = malloc(sizeof(struct vm_rg_struct));

	newrg->rg_start = current_vma->sbrk;  // malloc a new region at the sbrk
	newrg->rg_end = newrg->rg_start + size;
	newrg->rg_next = NULL;

	/* END TODO */

	return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
	/* TODO validate the planned memory area is not overlapped */
	if (vmastart >= vmaend)
	{
		return -1;
	} // check for valid range of vma_rg
    struct vm_area_struct *vma = caller->krnl->mm->mmap; 
	if(vma == NULL) return -1;
	while(vma != NULL){
		if(vma->vm_id != vmaid){
			if(vmastart < vma->vm_end && vmaend > vma->vm_start){
				return -1;
			}
		}
		vma = vma->vm_next;
	}
	// struct vm_area_struct *cur_area = get_vma_by_num(caller->krnl->mm, vmaid);
	// if (cur_area == NULL)
	// {
	// 	return -1;
	// }

	// while (vma != NULL)
	// {
	// 	if (vma != cur_area && OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end))
	// 	{
	// 		return -1;
	// 	}
	// 	vma = vma->vm_next;
	// }

	/* End TODO*/

	return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
	//struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));

	/* TOTO with new address scheme, the size need to be aligned 
	 *      the raw inc_sz maybe not fit pagesize
	 */

	//addr_t inc_amt;

	int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz); // align to page size (integer num of pages)
	int incnumpage =  inc_amt / PAGING_PAGESZ;

	/* TODO Validate overlap of obtained region */
	// if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
	// 	return -1; /*Overlap and failed allocation */

	/* TODO: Obtain the new vm area based on vmaid */
	// cur_vma->vm_end... 
	// inc_limit_ret...
	/* The obtained vm area (only)
	 * now will be alloc real ram region 
	 */

	
	struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
	if(cur_vma == NULL) return -1;
	
	struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
	if(area == NULL) return -1;
	
	if(validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0){
		free(area);
		return -1;
	}
	
	int old_end = cur_vma->sbrk;
	cur_vma->sbrk += inc_sz; // increase the sbrk by raw size
	if (cur_vma->sbrk > cur_vma->vm_end) {
        cur_vma->vm_end += inc_amt;
    }
	struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
	if(newrg == NULL){
		free(area);
		return -1;
	}
	
	if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage , newrg) < 0){
		free(area);
		free(newrg);
		return -1; /* Map the memory to MEMRAM */
	}
		

	newrg->rg_start = old_end;
	newrg->rg_end = cur_vma->sbrk;
	newrg->rg_next = NULL;

	enlist_vm_rg_node(&cur_vma->vm_freerg_list, newrg);
	free(area);
	return 0;
}

// #endif