#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>

#ifdef POPCORN_X86
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
const int SYS_sched_migrate = 356;

#elif defined(POPCORN_ARM)
struct regset_aarch64 {
	/* Stack pointer & program counter */
	void* sp;
	void* pc;

	/* General purpose registers */
	unsigned long x[31];

	/* FPU/SIMD registers */
	unsigned __int128 v[32];
};
const int SYS_sched_migrate = 285;

#elif defined(POPCORN_PPC)

const int SYS_sched_migrate = 285;
#else
#error No architecture is specified. Check the Makefile
#endif

void popcorn_migrate_this(int nid)
{
	pid_t tid = syscall(SYS_gettid);

#ifdef POPCORN_X86
	struct regset_x86_64 regs = {};

	asm volatile ("mov %%r15, %0" : "=r"(regs.r15));
	asm volatile ("mov %%r14, %0" : "=r"(regs.r14));
	asm volatile ("mov %%r13, %0" : "=r"(regs.r13));
	asm volatile ("mov %%r12, %0" : "=r"(regs.r12));
	asm volatile ("mov %%rbp, %0" : "=r"(regs.rbp));
	asm volatile ("mov %%rbx, %0" : "=r"(regs.rbx));
#elif defined(POPCORN_ARM)
	struct regset_aarch64 regs = {};
#endif

	// regs.rip = loop;
	syscall(SYS_sched_migrate, tid, nid, &regs);
}
