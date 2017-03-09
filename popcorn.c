#include <sys/syscall.h>
#include <fcntl.h>

struct regset_x86_64 {
	/* Program counter/instruction pointer */
	void* rip;

	/* General purpose registers */
	unsigned long rax, rdx, rcx, rbx,
			 rsi, rdi, rbp, rsp,
			 r8, r9, r10, r11,
			 r12, r13, r14, r15;

	/* Multimedia-extension (MMX) registers */
	unsigned long mmx[8];

	/* Streaming SIMD Extension (SSE) registers */
	unsigned __int128 xmm[16];

	/* x87 floating point registers */
	long double st[8];

	/* Segment registers */
	unsigned int cs, ss, ds, es, fs, gs;

	/* Flag register */
	unsigned long rflags;
};


void popcorn_migrate_this(int nid)
{
	struct regset_x86_64 regs = {};
	pid_t tid = syscall(SYS_gettid);

	asm volatile ("mov %%r15, %0" : "=r"(regs.r15));
	asm volatile ("mov %%r14, %0" : "=r"(regs.r14));
	asm volatile ("mov %%r13, %0" : "=r"(regs.r13));
	asm volatile ("mov %%r12, %0" : "=r"(regs.r12));
	asm volatile ("mov %%rbp, %0" : "=r"(regs.rbp));
	asm volatile ("mov %%rbx, %0" : "=r"(regs.rbx));

	// regs.rip = loop;
	syscall(356, tid, nid, &regs);
}
