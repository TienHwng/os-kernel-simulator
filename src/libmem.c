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
 * System Library
 * Memory Module Library libmem.c 
 */

#include "string.h"
#include "mm.h"
#include "mm64.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  /*Allocate at the toproof */
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct rgnode;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  int inc_sz = 0;

  // 1. Thử tìm vùng nhớ đã free trước đó
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->krnl->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->krnl->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* 2. Nếu không có, phải mở rộng vùng nhớ (Increase Limit) */
  
  int old_sbrk = cur_vma->sbrk;

  /* System Call để xin nới rộng bộ nhớ */
  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;

#ifdef MM64
  // Với MM64, align size theo page size 64-bit
  inc_sz = size; 
  regs.a3 = inc_sz;
#else
  // Với 32-bit, align theo page size thường
  inc_sz = PAGING_PAGE_ALIGNSZ(size);
  regs.a3 = inc_sz;
#endif  

  if (syscall(caller->krnl, caller->pid, 17, &regs) != 0) {
      printf("Alloc failed: Out of memory\n");
      pthread_mutex_unlock(&mmvm_lock);
      return -1;
  }

  /* Successful increase limit */
  caller->krnl->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->krnl->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  // Cập nhật sbrk cục bộ (nếu syscall chưa làm việc này trên struct user-space)
  cur_vma->sbrk += size; 

  *alloc_addr = old_sbrk;

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  pthread_mutex_lock(&mmvm_lock);

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  struct vm_rg_struct *rgnode = get_symrg_byid(caller->krnl->mm, rgid);

  if (rgnode == NULL || (rgnode->rg_start == 0 && rgnode->rg_end == 0))
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  // Tạo node mới để đưa vào danh sách free
  struct vm_rg_struct *freerg_node = malloc(sizeof(struct vm_rg_struct));
  freerg_node->rg_start = rgnode->rg_start;
  freerg_node->rg_end = rgnode->rg_end;
  freerg_node->rg_next = NULL;

  // Xóa thông tin vùng nhớ đang dùng
  rgnode->rg_start = 0;
  rgnode->rg_end = 0;
  rgnode->rg_next = NULL;

  /* enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->krnl->mm, freerg_node);

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*liballoc - PAGING-based allocate a region memory */
int liballoc(struct pcb_t *proc, addr_t size, uint32_t reg_index)
{
  addr_t addr;

  int val = __alloc(proc, 0, reg_index, size, &addr);
  if (val == -1)
  {
    return -1;
  }
  
  // Lưu địa chỉ đã cấp phát vào thanh ghi (theo logic máy ảo)
  proc->regs[reg_index] = addr;

#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); 
#endif
#endif

  return val;
}

/*libfree - PAGING-based free a region memory */
int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  int val = __free(proc, 0, reg_index);
  if (val == -1)
  {
    return -1;
  }
  
  // Reset thanh ghi
  proc->regs[reg_index] = 0;

#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); 
#endif
#endif
  return val;
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = pte_get_entry(caller, pgn);

  // Nếu trang chưa có trong RAM (Present = 0) -> Xử lý Page Fault
  if (!PAGING_PAGE_PRESENT(pte))
  { 
    addr_t vicpgn, swpfpn, vicfpn;
    // addr_t vicpte; // unused
    int tgtfpn = -1; // Frame đích sẽ dùng cho trang hiện tại

    // 1. Thử tìm frame trống trong RAM trước
    if (MEMPHY_get_freefp(caller->krnl->mram, &tgtfpn) != 0) 
    {
        // RAM đầy: Phải tìm nạn nhân (Victim) để Swap Out
        if (find_victim_page(caller->krnl->mm, &vicpgn) == -1)
            return -1;

        // Lấy thông tin frame của nạn nhân
        uint32_t vic_pte_val = pte_get_entry(caller, vicpgn);
        vicfpn = PAGING_FPN(vic_pte_val);

        // Tìm chỗ trống dưới SWAP để đẩy nạn nhân xuống
        if (MEMPHY_get_freefp(caller->krnl->active_mswp, &swpfpn) == -1)
            return -1; // Swap cũng đầy -> Thua!

        // Copy frame nạn nhân từ RAM -> SWAP
        __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);

        // Cập nhật PTE của nạn nhân: Đánh dấu là Swapped
        pte_set_swap(caller, vicpgn, 0, swpfpn);

        // Frame của nạn nhân giờ thành frame đích cho trang hiện tại
        tgtfpn = vicfpn;
    }

    // 2. Xử lý trang hiện tại (Target Page)
    // Nếu trang này trước đó đã bị Swap xuống (Swapped = 1) -> Phải Swap In
    if (PAGING_PTE_SWAPPED(pte)) {
        addr_t old_swpfpn = PAGING_SWP(pte);
        
        // Copy từ SWAP lên RAM
        __swap_cp_page(caller->krnl->active_mswp, old_swpfpn, caller->krnl->mram, tgtfpn);
        
        // Trả lại frame dưới SWAP (vì đã lôi lên RAM rồi)
        MEMPHY_put_freefp(caller->krnl->active_mswp, old_swpfpn);
    } 
    
    // 3. Cập nhật PTE của trang hiện tại: Trỏ vào frame RAM mới
    pte_set_fpn(caller, pgn, tgtfpn);

    // Thêm vào danh sách FIFO để quản lý thay thế trang sau này
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(pte_get_entry(caller,pgn));
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn * PAGING_PAGESZ) + off;

  // Gọi System Call IO READ
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  regs.a3 = 0;
  
  if (syscall(caller->krnl, caller->pid, 17, &regs) != 0)
      return -1;

  *data = (BYTE)regs.a3;
  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn * PAGING_PAGESZ) + off;

  // Gọi System Call IO WRITE
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;

  if (syscall(caller->krnl, caller->pid, 17, &regs) != 0)
      return -1;

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->krnl->mm, rgid);
  // struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (currg == NULL || currg->rg_start == 0) 
      return -1;

  if (pg_getval(caller->krnl->mm, currg->rg_start + offset, data, caller) != 0)
      return -1;

  return 0;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    addr_t offset,    // Source address = [source] + [offset]
    uint32_t* destination)
{
  BYTE data;
  // Lấy rgid từ giá trị thanh ghi nguồn
  // Lưu ý: source ở đây là index của bảng symbol, hoặc thanh ghi chứa địa chỉ
  // Theo logic đề bài: source là rgid
  int val = __read(proc, 0, source, offset, &data);

  *destination = (uint32_t)data;
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); 
#endif
#endif

  return val;
}

/*__write - write a region memory */
int __write(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->krnl->mm, rgid);
  
  // struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  if (currg == NULL) /* Invalid memory identify */
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  if (pg_setval(caller->krnl->mm, currg->rg_start + offset, value, caller) != 0) {
      pthread_mutex_unlock(&mmvm_lock);
      return -1;
  }

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    addr_t offset)
{
  int val = __write(proc, 0, destination, offset, data);
  if (val == -1)
  {
    return -1;
  }
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); 
#endif
  // MEMPHY_dump(proc->krnl->mram);
#endif

  return val;
}

/*free_pcb_memphy - collect all memphy of pcb */
int free_pcb_memph(struct pcb_t *caller)
{
  pthread_mutex_lock(&mmvm_lock);
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = pte_get_entry(caller, pagenum); // Sử dụng hàm getter thay vì truy cập trực tiếp

    if (PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
    }
    else if (PAGING_PTE_SWAPPED(pte))
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->krnl->active_mswp, fpn);
    }
  }

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, addr_t *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;

  if (!pg)
  {
    return -1;
  }
  
  // Tìm phần tử cuối cùng của danh sách FIFO (là trang vào sớm nhất -> Victim)
  struct pgn_t *prev = NULL;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  
  *retpgn = pg->pgn;

  // Xóa node khỏi danh sách
  if (prev) {
      prev->pg_next = NULL;
  } else {
      mm->fifo_pgn = NULL; // Danh sách chỉ có 1 phần tử
  }

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        {                                /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      return 0; // Found and allocated
    }
    else
    {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  return -1; // Not found
}

// #endif