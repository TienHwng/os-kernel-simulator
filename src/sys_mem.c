/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "os-mm.h"
#include "syscall.h"
#include "libmem.h"
#include "queue.h"
#include <stdlib.h>

#ifdef MM64
#include "mm64.h"
#else
#include "mm.h"
#endif

// typedef char BYTE;   // nếu BYTE chưa được định nghĩa ở nơi khác thì mở comment dòng này

/*
 * System call handler cho SYSCALL 17: SYSMEM_OP
 * - krnl : kernel context (chứa ready_queue, running_list, mram, ...)
 * - pid  : PID của tiến trình gọi syscall
 * - regs : thanh ghi syscall (a1 = memop, a2/a3 = tham số)
 */
int __sys_memmap(struct krnl_t *krnl, uint32_t pid, struct sc_regs *regs)
{
    int  memop = regs->a1;
    BYTE value;

    /* Tìm PCB tương ứng pid trong running_list của kernel */
    struct pcb_t   *caller       = NULL;
    struct queue_t *running_list = krnl->running_list;   /* running_list là con trỏ */

    if (running_list != NULL) {
        for (int i = 0; i < running_list->size; i++) {
            struct pcb_t *proc = running_list->proc[i];
            if (proc != NULL && proc->pid == pid) {
                caller = proc;
                break;
            }
        }
    }

    if (caller == NULL) {
        /* Không tìm được process có pid tương ứng */
        return -1;
    }

    switch (memop) {
    case SYSMEM_MAP_OP:
        /* Dummy mapping bằng page directory, không cấp phát frame thực */
        vmap_pgd_memset(caller, regs->a2, regs->a3);
        break;

    case SYSMEM_INC_OP:
        /* Tăng giới hạn VMA (heap/break pointer) của tiến trình */
        inc_vma_limit(caller, regs->a2, regs->a3);
        break;

    case SYSMEM_SWP_OP:
        /* Hoán đổi trang (swap in/out) cho tiến trình caller */
        __mm_swap_page(caller, regs->a2, regs->a3);
        break;

    case SYSMEM_IO_READ:
        /* Đọc 1 byte trực tiếp từ physical memory của kernel */
        MEMPHY_read(krnl->mram, regs->a2, &value);
        regs->a3 = value;   /* trả kết quả qua thanh ghi a3 */
        break;

    case SYSMEM_IO_WRITE:
        /* Ghi 1 byte trực tiếp xuống physical memory của kernel */
        MEMPHY_write(krnl->mram, regs->a2, regs->a3);
        break;

    default:
        printf("Memop code: %d\n", memop);
        break;
    }

    return 0;
}
