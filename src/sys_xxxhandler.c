
#include "common.h"
#include "stdio.h"
#include "syscall.h"

int __sys_xxxhandler(struct krnl_t *krnl, uint32_t pid, struct sc_regs *regs) {
	/* TODO: implement syscall job */
	printf("The first system call parameter %d\n", regs->a1);
	return 0;
}