/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#if defined(MM64)

// PTE: page table entry, FPN: frame pagging number
// dirty bit =0 (clean), =1 (dirty)

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
			 int pre,    	// present
			 addr_t fpn, 	// FPN
			 int drt,    	// dirty
			 int swp,    	// swap
			 int swptyp, 	// swap type
			 addr_t swpoff) // swap offset
{
	if (pre != 0) { 
		if (swp == 0) { // Non swap ~ page online
			if (fpn == 0)
			return -1;  // Invalid setting

			/* Valid setting with FPN */
			SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
			CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
			CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

			SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
		}
		else
		{ 
			// page swapped
			SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
			SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
			CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

			SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
			SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
		}
	}

	return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Extract page direactories */
	// *pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	// *p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	// *pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	// *pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	// *pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */

	*pgd = PAGING64_ADDR_PGD(addr);
    *p4d = PAGING64_ADDR_P4D(addr);
    *pud = PAGING64_ADDR_PUD(addr);
    *pmd = PAGING64_ADDR_PMD(addr);
    *pt  = PAGING64_ADDR_PT(addr); 
	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
							   pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
	struct krnl_t *krnl = caller->krnl; 

	addr_t index_pgd=0;
	addr_t index_p4d=0;
	addr_t index_pud=0;
	addr_t index_pmd=0;
	addr_t index_pt=0;
	
	// dummy pte alloc to avoid runtime error
	//  1 2 3 4 5
	//    2  
	//			5	
	//  1
	//	  	 4	
	#ifdef MM64	
	/* Get value from the system */
	/* TODO Perform multi-level page mapping */
	//... krnl->mm->pgd
	//... krnl->mm->pt
	get_pd_from_pagenum(pgn, &index_pgd, &index_p4d, &index_pud, &index_pmd, &index_pt);
	addr_t pgd_entry = krnl->mm->pgd[index_pgd];
	if(pgd_entry == 0) return 0;
	addr_t *p4d_base = (addr_t *)(pgd_entry);

	addr_t p4d_entry = p4d_base[index_p4d];
	if(p4d_entry == 0) return 0;
	addr_t *pud_base = (addr_t *)(p4d_entry);

	addr_t pud_entry = pud_base[index_pud];
	if(pud_entry ==0) return 0;
	addr_t *pmd_base = (addr_t *)(pud_entry);

	addr_t pmd_entry = pmd_base[index_pmd];
	if(pmd_entry ==0 ) return 0;
	addr_t *pt_base = (addr_t *)(pmd_entry);

#else
	pte = &krnl->mm->pgd[pgn];
#endif
	
	SETBIT(pt_base[index_pt], PAGING_PTE_PRESENT_MASK);
	SETBIT(pt_base[index_pt], PAGING_PTE_SWAPPED_MASK);

	SETVAL(pt_base[index_pt], swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
	SETVAL(pt_base[index_pt], swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

	return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn) // set frame page number
{
	struct krnl_t *krnl = caller->krnl;

	addr_t index_pgd=0;
	addr_t index_p4d=0;
	addr_t index_pud=0;
	addr_t index_pmd=0;
	addr_t index_pt=0;
	
	// dummy pte alloc to avoid runtime error
	// pte = malloc(sizeof(addr_t)); // cần thì bật, tạm tắt để tránh leak memory
#ifdef MM64	
	/* Get value from the system */
	/* TODO Perform multi-level page mapping */
	get_pd_from_pagenum(pgn, &index_pgd, &index_p4d, &index_pud, &index_pmd, &index_pt);
	//... krnl->mm->pgd
	//... krnl->mm->pt
	//pte = &krnl->mm->pt;
	
	addr_t pte = pte_get_entry(caller, pgn); // multilevel_mapping performed inside pte_get_entry()
	addr_t pgd_entry = krnl->mm->pgd[index_pgd];
	if(pgd_entry == 0) return 0;
	addr_t *p4d_base = (addr_t *)(pgd_entry);

	addr_t p4d_entry = p4d_base[index_p4d];
	if(p4d_entry == 0) return 0;
	addr_t *pud_base = (addr_t *)(p4d_entry);

	addr_t pud_entry = pud_base[index_pud];
	if(pud_entry ==0) return 0;
	addr_t *pmd_base = (addr_t *)(pud_entry);

	addr_t pmd_entry = pmd_base[index_pmd];
	if(pmd_entry ==0 ) return 0;
	addr_t *pt_base = (addr_t *)(pmd_entry);


#else
	pte = &krnl->mm->pgd[pgn];
#endif

	SETBIT(pt_base[index_pt], PAGING_PTE_PRESENT_MASK);
	CLRBIT(pt_base[index_pt], PAGING_PTE_SWAPPED_MASK);

	SETVAL(pt_base[index_pt], fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
	
	return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
	struct krnl_t *krnl = caller->krnl;
    struct mm_struct *mm = krnl->mm;

    addr_t idx_pgd, idx_p4d, idx_pud, idx_pmd, idx_pt;
    get_pd_from_pagenum(pgn, &idx_pgd, &idx_p4d, &idx_pud, &idx_pmd, &idx_pt);

    if (mm == NULL)
        return 0;

    if (mm->pgd[idx_pgd] == 0) {
        addr_t *new_p4d = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_p4d == NULL)
            return 0;

        printf("Creating new p4d at pgd[%ld]= %p\n\n", idx_pgd, new_p4d);
        mm->pgd[idx_pgd] = (addr_t)new_p4d;
    }
    addr_t *p4d_base = (addr_t *)mm->pgd[idx_pgd];

    if (p4d_base[idx_p4d] == 0) {
        addr_t *new_pud = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_pud == NULL)
            return 0;

        // printf("Creating new pud at p4d[%ld]= %p\n\n", idx_p4d, new_pud);
        p4d_base[idx_p4d] = (addr_t)new_pud;
    }
    addr_t *pud_base = (addr_t *)p4d_base[idx_p4d];

    if (pud_base[idx_pud] == 0) {
        addr_t *new_pmd = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_pmd == NULL)
            return 0;
        pud_base[idx_pud] = (addr_t)new_pmd;
    }
    addr_t *pmd_base = (addr_t *)pud_base[idx_pud];

    if (pmd_base[idx_pmd] == 0) {
        addr_t *new_pt = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_pt == NULL)
            return 0;
        pmd_base[idx_pmd] = (addr_t)new_pt;
    }
	
	addr_t *pt_base = (addr_t *)pmd_base[idx_pmd];

	return pt_base[idx_pt];
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, uint32_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
    struct mm_struct *mm = krnl->mm;

    addr_t idx_pgd, idx_p4d, idx_pud, idx_pmd, idx_pt;
    get_pd_from_pagenum(pgn, &idx_pgd, &idx_p4d, &idx_pud, &idx_pmd, &idx_pt);

    if (mm == NULL)
        return -1;

    if (mm->pgd[idx_pgd] == 0) {
        addr_t *new_p4d = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_p4d == NULL)
            return -1;

        printf("Creating new p4d at pgd[%ld]= %p\n\n", idx_pgd, new_p4d);
        mm->pgd[idx_pgd] = (addr_t)new_p4d;
    }
    addr_t *p4d_base = (addr_t *)mm->pgd[idx_pgd];

    if (p4d_base[idx_p4d] == 0) {
        addr_t *new_pud = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_pud == NULL)
            return -1;

        // printf("Creating new pud at p4d[%ld]= %p\n\n", idx_p4d, new_pud);
        p4d_base[idx_p4d] = (addr_t)new_pud;
    }
    addr_t *pud_base = (addr_t *)p4d_base[idx_p4d];

    if (pud_base[idx_pud] == 0) {
        addr_t *new_pmd = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_pmd == NULL)
            return -1;
        pud_base[idx_pud] = (addr_t)new_pmd;
    }
    addr_t *pmd_base = (addr_t *)pud_base[idx_pud];

    if (pmd_base[idx_pmd] == 0) {
        addr_t *new_pt = calloc(PAGING64_TABLE_ENTRIES, sizeof(addr_t));
        if (new_pt == NULL)
            return -1;
        pmd_base[idx_pmd] = (addr_t)new_pt;
    }
	
	addr_t *pt_base = (addr_t *)pmd_base[idx_pmd];

	pt_base[idx_pt] = pte_val;
	return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                    // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
    int pgit = 0;               // page iterator
    uint64_t pattern = 0xdeadbeef; // pattern to set
    struct krnl_t *krnl = caller->krnl;

    // Kiểm tra tính hợp lệ cơ bản
    if (addr % PAGING64_PAGESZ != 0) return -1;
    if (pgnum <= 0) return -1;

    // VÒNG LẶP: Duyệt qua từng trang một
    for (pgit = 0; pgit < pgnum; pgit++) 
    {
        // 1. Tính địa chỉ ảo của trang hiện tại
        addr_t cur_addr = addr + (pgit * PAGING64_PAGESZ);

        // 2. Khai báo các biến index
        addr_t index_pgd = 0;
        addr_t index_p4d = 0;
        addr_t index_pud = 0;
        addr_t index_pmd = 0;
        addr_t index_pt = 0;

        // 3. Lấy bộ chỉ số (index) cho trang hiện tại
        // Việc gọi hàm này trong vòng lặp giúp tự động xử lý việc chuyển sang bảng khác
        // khi index bị tràn (vượt quá 511).
        get_pd_from_address(cur_addr, &index_pgd, &index_p4d, &index_pud, &index_pmd, &index_pt);

        // 4. Leo cây phân trang để tìm pt_table
        // --- LEVEL 5: PGD ---
        addr_t pgd_entry = krnl->mm->pgd[index_pgd];
        if (pgd_entry == 0) return -1; // Chưa cấp phát -> Lỗi
        addr_t *p4d_table = (addr_t *)(pgd_entry);

        // --- LEVEL 4: P4D ---
        addr_t p4d_entry = p4d_table[index_p4d];
        if (p4d_entry == 0) return -1;
        addr_t *pud_table = (addr_t *)(p4d_entry);

        // --- LEVEL 3: PUD ---
        addr_t pud_entry = pud_table[index_pud];
        if (pud_entry == 0) return -1;
        addr_t *pmd_table = (addr_t *)(pud_entry);

        // --- LEVEL 2: PMD ---
        addr_t pmd_entry = pmd_table[index_pmd];
        if (pmd_entry == 0) return -1;
        addr_t *pt_table = (addr_t *)(pmd_entry);

        // --- LEVEL 1: PT ---
        // 5. Ghi pattern vào đúng vị trí
        pt_table[index_pt] = pattern;
    }

    return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
					addr_t addr,                       // start address which is aligned to pagesz
					int pgnum,                      // num of mapping page
					struct framephy_struct *frames, // list of the mapped frames
					struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
	struct framephy_struct *fpit;
	int pgit = 0;
	addr_t pgn ;
	/* TODO: update the rg_end and rg_start of ret_rg 
	//ret_rg->rg_end =  ....
	//ret_rg->rg_start = ...
	//ret_rg->vmaid = ...
	 */
	ret_rg->rg_start = addr;
	ret_rg->rg_end = addr + pgnum*PAGING64_PAGESZ;
	// ret_rg->rg_next = NULL;

	/* TODO map range of frame to address space
	 *      [addr to addr + pgnum*PAGING_PAGESZ
	 *      in page table caller->krnl->mm->pgd,
	 *                    caller->krnl->mm->pud...
	 *                    ...
	 */
	for(fpit = frames, pgit = 0; pgit < pgnum && fpit != NULL; pgit++, fpit = fpit->fp_next)  //mapping pages to frames
	{
		pgn = (addr >> PAGING64_ADDR_PT_LOBIT) + pgit; // exlude offset bits
		pte_set_fpn(caller, pgn, fpit->fpn);

		enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
	}
	/* Tracking for later page replacement activities (if needed)
	* Enqueue new usage page */

	return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
	struct framephy_struct *newfp_str = NULL;
    struct framephy_struct *tmp = NULL;
    addr_t fpn;
    int pgit;

    /* Basic argument validation */
    if (req_pgnum <= 0)
        return -1;

    /* Check if requested size exceeds total RAM size (in bytes) */
    if ((addr_t)req_pgnum * PAGING64_PAGESZ > caller->krnl->mram->maxsz)
        return -3000; /* request size exceeds max RAM */

    /* Initialize output list if caller didn't */
    if (frm_lst == NULL)
        return -1;

    /* We build list in LIFO order at *frm_lst */
    for (pgit = 0; pgit < req_pgnum; pgit++)
    {
        /* Try to get a free frame from RAM */
        if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) != 0)
        {
            /* Out of free frames: rollback frames we already allocated */
            tmp = *frm_lst;
            while (tmp != NULL)
            {
                struct framephy_struct *next = tmp->fp_next;

                /* Return the frame back to free list in RAM */
                MEMPHY_put_freefp(caller->krnl->mram, tmp->fpn);

                free(tmp);
                tmp = next;
            }
            *frm_lst = NULL;
            return -3000; /* out of memory */
        }

        /* Allocate metadata node for this frame */
        newfp_str = (struct framephy_struct *)malloc(sizeof(struct framephy_struct));
        if (newfp_str == NULL)
        {
            /* Metadata allocation failed: rollback frames already taken */
            tmp = *frm_lst;
            while (tmp != NULL)
            {
                struct framephy_struct *next = tmp->fp_next;

                MEMPHY_put_freefp(caller->krnl->mram, tmp->fpn);

                free(tmp);
                tmp = next;
            }
            *frm_lst = NULL;

            /* Also return the last frame we just reserved but couldn't wrap */
            MEMPHY_put_freefp(caller->krnl->mram, fpn);

            return -3000;
        }

        newfp_str->fpn = fpn;
        newfp_str->fp_next = *frm_lst;
        *frm_lst = newfp_str;
    }

    return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
	struct framephy_struct *frm_lst = NULL;
	addr_t ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);
	// int pgnum = incpgnum;

	/*@bksysnet: author provides a feasible solution of getting frames
	 *FATAL logic in here, wrong behaviour if we have not enough page
	 *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
	 *Don't try to perform that case in this simple work, it will result
	 *in endless procedure of swap-off to get frame and we have not provide
	 *duplicate control mechanism, keep it simple
	 */
	
	// ret_alloc = alloc_pages_range(caller, pgnum, &frm_lst);

	if (ret_alloc < 0 && ret_alloc != -3000) return -1;

	/* Out of memory */
	if (ret_alloc == -3000)
	{
		return -1;
	}

	/* it leaves the case of memory is enough but half in ram, half in swap
	* do the swaping all to swapper to get the all in ram */
	if(vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg) < 0){
		struct framephy_struct *freefp_str;
		while(frm_lst != NULL){
			freefp_str = frm_lst;
			frm_lst = frm_lst->fp_next;

			//MEMPHY_put_freefp(caller->krnl->mram, freefp_str->fpn);
			free(freefp_str);
		}
		return -1;
	}

	struct framephy_struct *freefp_str;
	while(frm_lst != NULL){
		freefp_str = frm_lst;
		frm_lst = frm_lst->fp_next;
		free(freefp_str);
	}

	return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
				   struct memphy_struct *mpdst, addr_t dstfpn)
{
	int cellidx;
	addr_t addrsrc, addrdst;
	for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
	{
		addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
		addrdst = dstfpn * PAGING_PAGESZ + cellidx;

		BYTE data;
		MEMPHY_read(mpsrc, addrsrc, &data);
		MEMPHY_write(mpdst, addrdst, data);
	}

	return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
	/* TODO init page table directory */
	//mm->pgd = ...
	//mm->p4d = ...
	//mm->pud = ...
	//mm->pmd = ...
	//mm->pt = ...
	struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
	if(vma0 == NULL){
		return -1;
	}
	mm->pgd = malloc(PAGING64_MAX_PGN * sizeof(addr_t));
	if( mm->pgd == NULL){
		free(vma0);
		return -1;
	}
	
	/* By default the owner comes with at least one vma */
	vma0->vm_id = 0;
	vma0->vm_start = 0;		
	vma0->vm_end = vma0->vm_start;
	vma0->sbrk = vma0->vm_start; //system break set to start due to no ram allocated yet

	struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
	if(first_rg == NULL){
		free (vma0);
		free(mm->pgd);
		return -1;
	}
	enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

	/* TODO update VMA0 next */
	vma0->vm_next = NULL;

	/* Point vma owner backward */
	vma0->vm_mm = mm; 

	/* TODO: update mmap */
	mm->mmap = vma0;

	return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
	struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

	rgnode->rg_start = rg_start;
	rgnode->rg_end = rg_end;
	rgnode->rg_next = NULL;

	return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
	rgnode->rg_next = *rglist;
	*rglist = rgnode;

	return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn) // insert a new node to te head of pgn_t list
{
	struct pgn_t *pnode = malloc(sizeof(struct pgn_t));

	pnode->pgn = pgn;
	pnode->pg_next = *plist;
	*plist = pnode;

	return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
	struct framephy_struct *fp = ifp;

	printf("print_list_fp: ");
	if (fp == NULL) { printf("NULL list\n"); return -1;}
	printf("\n");
	while (fp != NULL)
	{
		printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
		fp = fp->fp_next;
	}
	printf("\n");
	return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
	struct vm_rg_struct *rg = irg;

	printf("print_list_rg: ");
	if (rg == NULL) { printf("NULL list\n"); return -1; }
	printf("\n");
	while (rg != NULL)
	{
		printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
		rg = rg->rg_next;
	}
	printf("\n");
	return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
	struct vm_area_struct *vma = ivma;

	printf("print_list_vma: ");
	if (vma == NULL) { printf("NULL list\n"); return -1; }
	printf("\n");
	while (vma != NULL)
	{
		printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
		vma = vma->vm_next;
	}
	printf("\n");
	return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
	printf("print_list_pgn: ");
	if (ip == NULL) { printf("NULL list\n"); return -1; }
	printf("\n");
	while (ip != NULL)
	{
		printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
		ip = ip->pg_next;
	}
	printf("n");
	return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
	if(caller == NULL){
        printf("No caller provided\n");
        return -1;
    }
    
    // Giữ nguyên logic lấy end nếu cần (dù output thầy có thể chỉ in dựa trên start)
    if (end == -1) {
        struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, 0);
        if (cur_vma) end = cur_vma->vm_end;
        else end = 0; 
    }

    struct mm_struct *mm = caller->krnl->mm;
    addr_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;

    // 1. In tiêu đề đúng như output mẫu
    printf("print_pgtbl:\n");

    // 2. Không dùng vòng lặp for duyệt hết các trang nữa.
    // Chỉ lấy thông tin cấu trúc phân trang tại địa chỉ bắt đầu (start)
    // để chứng minh cây phân trang đã được tạo.
    
    if (mm->pgd != NULL) {
        // Lấy bộ chỉ số (index) cho địa chỉ start
        get_pd_from_address(start, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

        // --- TRAVERSE TREE (Đi bộ qua cây để lấy giá trị các bảng) ---
        
        // Level 5: PGD
        // Giá trị tại pgd[idx] chính là địa chỉ của bảng P4D
        addr_t pgd_val = mm->pgd[pgd_idx];
        
        addr_t p4d_val = 0;
        addr_t pud_val = 0;
        addr_t pmd_val = 0;

        // Level 4: P4D
        if (pgd_val != 0) {
            addr_t *p4d_base = (addr_t *)pgd_val;
            p4d_val = p4d_base[p4d_idx];

            // Level 3: PUD
            if (p4d_val != 0) {
                addr_t *pud_base = (addr_t *)p4d_val;
                pud_val = pud_base[pud_idx];

                // Level 2: PMD
                if (pud_val != 0) {
                    addr_t *pmd_base = (addr_t *)pud_val;
                    pmd_val = pmd_base[pmd_idx];
                }
            }
        }

        // 3. In ra 1 dòng duy nhất theo format của thầy
        // Lưu ý: Output thầy dùng format P4g (có thể là typo của P4D), bạn cứ in giống hệt
        printf(" PDG=%016lx P4g=%016lx PUD=%016lx PMD=%016lx\n", 
               pgd_val, p4d_val, pud_val, pmd_val);
    }

    return 0;
}

#endif  //def MM64

// uint64_t temp = krnl->mm->pgd[pgit];
// if((temp & PAGING_PTE_PRESENT_MASK) || (temp & PAGING_PTE_SWAPPED_MASK)){
// 	printf("[PGN: %05ld] PTE: %016lx ", pgit, temp);
// 	if (temp & PAGING_PTE_SWAPPED_MASK){
// 		printf("(SWAPPED: Type=%ld, Off=%ld)", 
//         GETVAL(temp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT),
//         GETVAL(temp, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT));
// 	}else if(temp & PAGING_PTE_PRESENT_MASK){
// 		printf("(RAM: FPN=%ld)", 
//         GETVAL(temp, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT));
// 	}
// 	printf("\n");
// }