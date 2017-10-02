#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __x86_64__
#define SYSCALL_SCHED_MIGRATE	330
#define SYSCALL_SCHED_PROPOSE_MIGRATION		331
#define SYSCALL_SCHED_MIGRATION_PROPOSED	332
#define SYSCALL_GETTID 186
#elif __aarch64__
#define SYSCALL_SCHED_MIGRATE	285
#define SYSCALL_SCHED_PROPOSE_MIGRATION		286
#define SYSCALL_SCHED_MIGRATION_PROPOSED	287
#define SYSCALL_GETTID 178
#elif __powerpc64__
#define SYSCALL_SCHED_MIGRATE	379
#define SYSCALL_SCHED_PROPOSE_MIGRATION		380
#define SYSCALL_SCHED_MIGRATION_PROPOSED	381
#define SYSCALL_GETTID 207
#else
#error Does not support this architecture
#endif

void *popcorn_malloc(int size)
{
	void *p = NULL;
	posix_memalign(&p, 4096, size);
	return p;
}

int popcorn_gettid(void)
{
	return syscall(SYSCALL_GETTID);
}

#ifdef _OPENMP
int popcorn_omp_split(int tid, int threads, int start, int end, int *s, int *e)
{
	int N = end - start + 1;
	int step = N / threads;
	*s = start + tid * step;
	if (tid != threads - 1) {
		*e = start + (tid + 1) * step - 1;
	} else {
		*e = end;
	}
	return 0;
}
#endif

#ifdef __x86_64__
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

#elif __aarch64__
struct regset_aarch64 {
	/* Stack pointer & program counter */
	void* sp;
	void* pc;

	/* General purpose registers */
	unsigned long x[31];

	/* FPU/SIMD registers */
	unsigned __int128 v[32];
};

#elif __powerpc64__
struct regset_ppc {
	unsigned long nip;
	unsigned long msr;
	unsigned long ctr;
	unsigned long link;
	unsigned long xer;
	unsigned long ccr;

	unsigned long gpr[32];
	unsigned long fpr[32];

	unsigned long orig_gpr3;	/* Used for restarting system calls */
	unsigned long softe;		/* Soft enabled/disabled */
};
#else
#error No architecture is specified. Check the Makefile
#endif

int popcorn_migration_proposed(void)
{
	return syscall(SYSCALL_SCHED_MIGRATION_PROPOSED);
}

int popcorn_propose_migration(pid_t tid, int nid)
{
	return syscall(SYSCALL_SCHED_PROPOSE_MIGRATION, tid, nid);
}

#ifdef WAIT_FOR_DEBUGGER
int __wait_for_debugger = 1;
#endif

#ifdef __x86_64__
#define GET_REGISTER(x) \
		asm volatile ("mov %%"#x", %0" : "=m"(regs.x))
#define GET_XMM_REGISTER(x) \
		asm volatile ("movdqu %%xmm"#x", %0" : "=m"(regs.xmm[x]))
#define SET_XMM_REGISTER(x) \
		asm volatile ("movdqu %0, %%xmm"#x :: "m"(regs.xmm[x]))

void migrate(int nid, void (*callback_fn)(void *), void *callback_args)
{
	struct regset_x86_64 regs;

#ifdef DEBUG
	memset(&regs, 0xcd, sizeof(regs));
#endif

	GET_REGISTER(rax);
	GET_REGISTER(rdx);
	GET_REGISTER(rcx);
	GET_REGISTER(rbx);
	GET_REGISTER(rbp);
	GET_REGISTER(rsi);
	GET_REGISTER(rdi);
	GET_REGISTER(rsp);
	GET_REGISTER(r8);
	GET_REGISTER(r9);
	GET_REGISTER(r10);
	GET_REGISTER(r11);
	GET_REGISTER(r12);
	GET_REGISTER(r13);
	GET_REGISTER(r14);
	GET_REGISTER(r15);

	GET_REGISTER(cs);
	GET_REGISTER(ss);
	GET_REGISTER(ds);
	GET_REGISTER(es);
	GET_REGISTER(fs);
	GET_REGISTER(gs);
	asm volatile ("pushfq; popq %0" : "=m"(regs.rflags));

	GET_XMM_REGISTER(0);
	GET_XMM_REGISTER(1);
	GET_XMM_REGISTER(2);
	GET_XMM_REGISTER(3);
	GET_XMM_REGISTER(4);
	GET_XMM_REGISTER(5);
	GET_XMM_REGISTER(6);
	GET_XMM_REGISTER(7);
	GET_XMM_REGISTER(8);
	GET_XMM_REGISTER(9);
	GET_XMM_REGISTER(10);
	GET_XMM_REGISTER(11);
	GET_XMM_REGISTER(12);
	GET_XMM_REGISTER(13);
	GET_XMM_REGISTER(14);
	GET_XMM_REGISTER(15);

	asm volatile (
			"movq %1, %0"
		: "=m"(regs.rip)
		: "i"(&&migrated)
	);
	asm volatile ("mfence" ::: "memory");

	asm volatile (
			"mov %0, %%edi;"
			"mov %1, %%rsi;"
			"mov %2, %%eax;"
			"syscall;"
		:
		: "g"(nid), "g"(&regs), "i"(SYSCALL_SCHED_MIGRATE)
		: "edi", "rsi", "eax"
	);

migrated:
	asm volatile ("mfence" ::: "memory");
#ifdef WAIT_FOR_DEBUGGER
	while (__wait_for_debugger);
#endif

	SET_XMM_REGISTER(0);
	SET_XMM_REGISTER(1);
	SET_XMM_REGISTER(2);
	SET_XMM_REGISTER(3);
	SET_XMM_REGISTER(4);
	SET_XMM_REGISTER(5);
	SET_XMM_REGISTER(6);
	SET_XMM_REGISTER(7);
	SET_XMM_REGISTER(8);
	SET_XMM_REGISTER(9);
	SET_XMM_REGISTER(10);
	SET_XMM_REGISTER(11);
	SET_XMM_REGISTER(12);
	SET_XMM_REGISTER(13);
	SET_XMM_REGISTER(14);
	SET_XMM_REGISTER(15);

	if (callback_fn) callback_fn(callback_args);
	return;
}


#elif __aarch64__
#define GET_REGISTER(r) \
		asm volatile ("str x"#r", %0" : "=m"(regs.x[r]))
#define GET_FP_REGISTER(r) \
		asm volatile ("str q"#r", %0" : "=m"(regs.v[r]))
#define SET_FP_REGISTER(r) \
		asm volatile ("ldr q"#r", %0" : "=m"(regs.v[r]))

void migrate(int nid, void (*callback_fn)(void *), void *callback_args)
{
	struct regset_aarch64 regs;

#ifdef DEBUG
	memset(&regs, 0xcd, sizeof(regs));
#endif

	GET_REGISTER(0);
	GET_REGISTER(1);
	GET_REGISTER(2);
	GET_REGISTER(3);
	GET_REGISTER(4);
	GET_REGISTER(5);
	GET_REGISTER(6);
	GET_REGISTER(7);
	GET_REGISTER(8);
	GET_REGISTER(9);
	GET_REGISTER(10);
	GET_REGISTER(11);
	GET_REGISTER(12);
	GET_REGISTER(13);
	GET_REGISTER(14);
	GET_REGISTER(15);
	GET_REGISTER(16);
	GET_REGISTER(17);
	GET_REGISTER(18);
	GET_REGISTER(19);
	GET_REGISTER(20);
	GET_REGISTER(21);
	GET_REGISTER(22);
	GET_REGISTER(23);
	GET_REGISTER(24);
	GET_REGISTER(25);
	GET_REGISTER(26);
	GET_REGISTER(27);
	GET_REGISTER(28);
	GET_REGISTER(29);
	GET_REGISTER(30);

	GET_FP_REGISTER(0);
	GET_FP_REGISTER(1);
	GET_FP_REGISTER(2);
	GET_FP_REGISTER(3);
	GET_FP_REGISTER(4);
	GET_FP_REGISTER(5);
	GET_FP_REGISTER(6);
	GET_FP_REGISTER(7);
	GET_FP_REGISTER(8);
	GET_FP_REGISTER(9);
	GET_FP_REGISTER(10);
	GET_FP_REGISTER(11);
	GET_FP_REGISTER(12);
	GET_FP_REGISTER(13);
	GET_FP_REGISTER(14);
	GET_FP_REGISTER(15);
	GET_FP_REGISTER(16);
	GET_FP_REGISTER(17);
	GET_FP_REGISTER(18);
	GET_FP_REGISTER(19);
	GET_FP_REGISTER(20);
	GET_FP_REGISTER(21);
	GET_FP_REGISTER(22);
	GET_FP_REGISTER(23);
	GET_FP_REGISTER(24);
	GET_FP_REGISTER(25);
	GET_FP_REGISTER(26);
	GET_FP_REGISTER(27);
	GET_FP_REGISTER(28);
	GET_FP_REGISTER(29);
	GET_FP_REGISTER(30);
	GET_FP_REGISTER(31);
	/* SP */
	asm volatile (
			"mov x15, sp;"
			"str x15, %0;"
		: "=m"(regs.sp)
		:
		: "x15"
	);
	/* PC */
	asm volatile (
			"mov x15, %1;"
			"str x15, %0;"
		: "=m"(regs.pc)
		: "r"(&&migrated)
		: "x15"
	);

	/* SYSCALL */
	asm volatile (
			"mov w0, %w0;"
			"mov x1, %1;"
			"mov x8, %2;"
			"svc 0;"
		:
		: "r"(nid), "r"(&regs), "i"(SYSCALL_SCHED_MIGRATE)
		: "w0", "x1", "x8"
	);

migrated:
	asm volatile ("" ::: "memory");
#ifdef WAIT_FOR_DEBUGGER
	while (__wait_for_debugger);
#endif

	SET_FP_REGISTER(0);
	SET_FP_REGISTER(1);
	SET_FP_REGISTER(2);
	SET_FP_REGISTER(3);
	SET_FP_REGISTER(4);
	SET_FP_REGISTER(5);
	SET_FP_REGISTER(6);
	SET_FP_REGISTER(7);
	SET_FP_REGISTER(8);
	SET_FP_REGISTER(9);
	SET_FP_REGISTER(10);
	SET_FP_REGISTER(11);
	SET_FP_REGISTER(12);
	SET_FP_REGISTER(13);
	SET_FP_REGISTER(14);
	SET_FP_REGISTER(15);
	SET_FP_REGISTER(16);
	SET_FP_REGISTER(17);
	SET_FP_REGISTER(18);
	SET_FP_REGISTER(19);
	SET_FP_REGISTER(20);
	SET_FP_REGISTER(21);
	SET_FP_REGISTER(22);
	SET_FP_REGISTER(23);
	SET_FP_REGISTER(24);
	SET_FP_REGISTER(25);
	SET_FP_REGISTER(26);
	SET_FP_REGISTER(27);
	SET_FP_REGISTER(28);
	SET_FP_REGISTER(29);
	SET_FP_REGISTER(30);
	SET_FP_REGISTER(31);

	if (callback_fn) callback_fn(callback_args);
	return;
}

#elif __powerpc64__
#define GET_REGISTER(r) \
		asm volatile ("std "#r", %0" : "=m"(regs.gpr[r]))
#define GET_FP_REGISTER(r) \
		asm volatile ("stfd "#r", %0" : "=m"(regs.fpr[r]))
#define SET_FP_REGISTER(r) \
		asm volatile ("lfd "#r", %0" : : "m"(regs.fpr[r]))

void migrate(int nid, void (*callback_fn)(void *), void *callback_args)
{
	struct regset_ppc regs;

#ifdef DEBUG
	memset(&regs, 0xcd, sizeof(regs));
#endif

	GET_REGISTER(0);
	GET_REGISTER(1);
	GET_REGISTER(2);
	GET_REGISTER(3);
	GET_REGISTER(4);
	GET_REGISTER(5);
	GET_REGISTER(6);
	GET_REGISTER(7);
	GET_REGISTER(8);
	GET_REGISTER(9);
	GET_REGISTER(10);
	GET_REGISTER(11);
	GET_REGISTER(12);
	GET_REGISTER(13);
	GET_REGISTER(14);
	GET_REGISTER(15);
	GET_REGISTER(16);
	GET_REGISTER(17);
	GET_REGISTER(18);
	GET_REGISTER(19);
	GET_REGISTER(20);
	GET_REGISTER(21);
	GET_REGISTER(22);
	GET_REGISTER(23);
	GET_REGISTER(24);
	GET_REGISTER(25);
	GET_REGISTER(26);
	GET_REGISTER(27);
	GET_REGISTER(28);
	GET_REGISTER(29);
	GET_REGISTER(30);
	GET_REGISTER(31);
	GET_FP_REGISTER(0);
	GET_FP_REGISTER(1);
	GET_FP_REGISTER(2);
	GET_FP_REGISTER(3);
	GET_FP_REGISTER(4);
	GET_FP_REGISTER(5);
	GET_FP_REGISTER(6);
	GET_FP_REGISTER(7);
	GET_FP_REGISTER(8);
	GET_FP_REGISTER(9);
	GET_FP_REGISTER(10);
	GET_FP_REGISTER(11);
	GET_FP_REGISTER(12);
	GET_FP_REGISTER(13);
	GET_FP_REGISTER(14);
	GET_FP_REGISTER(15);
	GET_FP_REGISTER(16);
	GET_FP_REGISTER(17);
	GET_FP_REGISTER(18);
	GET_FP_REGISTER(19);
	GET_FP_REGISTER(20);
	GET_FP_REGISTER(21);
	GET_FP_REGISTER(22);
	GET_FP_REGISTER(23);
	GET_FP_REGISTER(24);
	GET_FP_REGISTER(25);
	GET_FP_REGISTER(26);
	GET_FP_REGISTER(27);
	GET_FP_REGISTER(28);
	GET_FP_REGISTER(29);
	GET_FP_REGISTER(30);
	GET_FP_REGISTER(31);

	/* PC */
	asm volatile(
			"mflr 4;"
			"bcl 20,31,$+4;"
			"mflr 5;"
			"addi 5,5,48;" /* Point to @migrated */
			"std 5,%0;"
			"mtlr 4;"
		: "=m" (regs.nip)
		:
		: "r4", "r5"
	);

	/* MSR */
	//asm volatile("mfmsr 4; std 4, %0" : "=m"(regs.msr) : : "r4");

	/* CTR */
	asm volatile("mfctr 4; std 4, %0" : "=m"(regs.ctr) : : "r4");

	/* LR */
	asm volatile("mflr 4; std 4, %0" : "=m"(regs.link) : : "r4");

	/* XER */
	//asm volatile("mfxer 4; std 4, %0" : "=m"(regs.xer) : : "r4");

	/* Syscall */
	asm volatile(
			"mr 3, %0;"
			"mr 4, %1;"
			"li 0, %2;"
			"sc;"
		:
		: "r"(nid), "r"(&regs), "i"(SYSCALL_SCHED_MIGRATE)
		: "r0", "r3", "r4"
	);

//migrated:
	asm volatile ("" ::: "memory");
#ifdef WAIT_FOR_DEBUGGER
	while (__wait_for_debugger);
#endif

	SET_FP_REGISTER(0);
	SET_FP_REGISTER(1);
	SET_FP_REGISTER(2);
	SET_FP_REGISTER(3);
	SET_FP_REGISTER(4);
	SET_FP_REGISTER(5);
	SET_FP_REGISTER(6);
	SET_FP_REGISTER(7);
	SET_FP_REGISTER(8);
	SET_FP_REGISTER(9);
	SET_FP_REGISTER(10);
	SET_FP_REGISTER(11);
	SET_FP_REGISTER(12);
	SET_FP_REGISTER(13);
	SET_FP_REGISTER(14);
	SET_FP_REGISTER(15);
	SET_FP_REGISTER(16);
	SET_FP_REGISTER(17);
	SET_FP_REGISTER(18);
	SET_FP_REGISTER(19);
	SET_FP_REGISTER(20);
	SET_FP_REGISTER(21);
	SET_FP_REGISTER(22);
	SET_FP_REGISTER(23);
	SET_FP_REGISTER(24);
	SET_FP_REGISTER(25);
	SET_FP_REGISTER(26);
	SET_FP_REGISTER(27);
	SET_FP_REGISTER(28);
	SET_FP_REGISTER(29);
	SET_FP_REGISTER(30);
	SET_FP_REGISTER(31);

	if (callback_fn) callback_fn(callback_args);
	return;
}
#endif
