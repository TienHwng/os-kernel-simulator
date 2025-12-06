/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

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

//typedef char BYTE;

int __sys_memmap(struct krnl_t *krnl, uint32_t pid, struct sc_regs* regs)
{
	int memop = regs->a1;
    BYTE value;
    int i;

    /* --- BẮT ĐẦU SỬA --- */
    struct pcb_t *caller = NULL;
    struct queue_t *q = krnl->running_list;

    // Duyệt qua danh sách running để tìm PCB có PID tương ứng
    // Lưu ý: Tùy vào cấu trúc queue_t của bạn (mảng hay linked-list). 
    // Dưới đây là code cho cấu trúc mảng (phổ biến nhất trong BTL này).
    for (i = 0; i < q->size; i++) {
        struct pcb_t *proc = q->proc[i];
        if (proc != NULL && proc->pid == pid) {
            caller = proc;
            break;
        }
    }

    if (caller == NULL) {
        return -1; // Không tìm thấy tiến trình
    }

    // Đảm bảo caller trỏ đúng về kernel hiện tại (đề phòng loader chưa gán)
    caller->krnl = krnl; 
    /* --- KẾT THÚC SỬA --- */

    switch (memop) {
    case SYSMEM_MAP_OP:
        // vmap_pgd_memset(caller, regs->a2, regs->a3); // Hàm này thường dùng init, cẩn thận khi gọi
        break;
    case SYSMEM_INC_OP:
        // Gọi hàm tăng bộ nhớ
        // regs->a2: vmaid (thường là 0)
        // regs->a3: size cần tăng
        inc_vma_limit(caller, regs->a2, regs->a3);
        break;
    case SYSMEM_SWP_OP:
        __mm_swap_page(caller, regs->a2, regs->a3);
        break;
    case SYSMEM_IO_READ:
        // Cần truyền đúng tham số cho MEMPHY_read
        if (MEMPHY_read(caller->krnl->mram, regs->a2, &value) == 0) {
             regs->a3 = value; // Trả giá trị đọc được về thanh ghi
        } else {
             regs->a3 = -1; // Đánh dấu lỗi
        }
        break;
    case SYSMEM_IO_WRITE:
        MEMPHY_write(caller->krnl->mram, regs->a2, regs->a3);
        break;
    default:
        printf("Memop code: %d\n", memop);
        break;
    }
   
    // KHÔNG ĐƯỢC free(caller) vì đây là tiến trình thật đang chạy!
    // free(caller); <--- Xóa dòng này đi
    
    return 0;
}


