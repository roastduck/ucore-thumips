#include <unistd.h>
#include <proc.h>
#include <syscall.h>
#include <trap.h>
#include <stdio.h>
#include <pmm.h>
#include <assert.h>

extern volatile int ticks;

static int
sys_exit(uint32_t arg[]) {
    int error_code = (int)arg[0];
    return do_exit(error_code);
}

static int
sys_fork(uint32_t arg[]) {
    struct trapframe *tf = current->tf;
    uintptr_t stack = tf->tf_regs.reg_r[MIPS_REG_SP];
    return do_fork(0, stack, tf);
}

static int
sys_wait(uint32_t arg[]) {
    int pid = (int)arg[0];
    int *store = (int *)arg[1];
    return do_wait(pid, store);
}

static int
sys_exec(uint32_t arg[]) {
    const char *name = (const char *)arg[0];
    size_t len = (size_t)arg[1];
    unsigned char *binary = (unsigned char *)arg[2];
    size_t size = (size_t)arg[3];
    return do_execve(name, len, binary, size);
}

static int
sys_yield(uint32_t arg[]) {
    return do_yield();
}

static int
sys_kill(uint32_t arg[]) {
    int pid = (int)arg[0];
    return do_kill(pid);
}

static int
sys_getpid(uint32_t arg[]) {
    return current->pid;
}

static int
sys_putc(uint32_t arg[]) {
    int c = (int)arg[0];
    kputchar(c);
    return 0;
}

static int
sys_pgdir(uint32_t arg[]) {
    print_pgdir();
    return 0;
}

static uint32_t
sys_gettime(uint32_t arg[]) {
    return (int)ticks;
}

static int (*syscalls[])(uint32_t arg[]) = {
  [SYS_exit]              sys_exit,
  [SYS_fork]              sys_fork,
  [SYS_wait]              sys_wait,
  [SYS_exec]              sys_exec,
  [SYS_yield]             sys_yield,
  [SYS_kill]              sys_kill,
  [SYS_getpid]            sys_getpid,
  [SYS_putc]              sys_putc,
  [SYS_pgdir]             sys_pgdir,
	[SYS_gettime]           sys_gettime,
};

#define NUM_SYSCALLS        ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void
syscall(void) {
    assert(current != NULL);
    struct trapframe *tf = current->tf;
    uint32_t arg[4];
    int num = tf->tf_regs.reg_r[MIPS_REG_V0];
    num -= SYSCALL_BASE;
    //kprintf("$ %d %d\n",current->pid, num);
    if (num >= 0 && num < NUM_SYSCALLS) {
        if (syscalls[num] != NULL) {
            arg[0] = tf->tf_regs.reg_r[MIPS_REG_A0];
            arg[1] = tf->tf_regs.reg_r[MIPS_REG_A1];
            arg[2] = tf->tf_regs.reg_r[MIPS_REG_A2];
            arg[3] = tf->tf_regs.reg_r[MIPS_REG_A3];
            tf->tf_regs.reg_r[MIPS_REG_V0] = syscalls[num](arg);
            return ;
        }
    }
    print_trapframe(tf);
    panic("undefined syscall %d, pid = %d, name = %s.\n",
            num, current->pid, current->name);
}
