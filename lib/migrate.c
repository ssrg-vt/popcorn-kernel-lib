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
#define SYSCALL_POPCORN_MIGRATE	434
#define SYSCALL_POPCORN_GET_THREAD_STATUS	435
#define SYSCALL_POPCORN_GET_NODE_INFO	436
#define SYSCALL_GETTID 186
#else
#error Does not support this architecture
#endif


int popcorn_gettid(void)
{
    return syscall( __NR_gettid);
}

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

#else
#error No architecture is specified. Check the Makefile
#endif


struct popcorn_thread_status;
int popcorn_get_status(struct popcorn_thread_status *status)
{
	return syscall(SYSCALL_POPCORN_GET_THREAD_STATUS, status);
}

struct popcorn_node_info;
int popcorn_get_node_info(int *current_nid, struct popcorn_node_info *info, int len)
{
	return syscall(SYSCALL_POPCORN_GET_NODE_INFO, current_nid, info, len);
}

#ifdef WAIT_FOR_DEBUGGER
static int __wait_for_debugger = 1;
#endif

#ifdef __x86_64__
#define GET_REGISTER(x) \
		asm volatile ("mov %%"#x", %0" : "=m"(regs.x))
#define SET_REGISTER(x) \
		asm volatile ("mov %0, %%"#x"" :: "m"(regs.x))
#define GET_XMM_REGISTER(x) \
		asm volatile ("movdqu %%xmm"#x", %0" : "=m"(regs.xmm[x]))
#define SET_XMM_REGISTER(x) \
		asm volatile ("movdqu %0, %%xmm"#x :: "m"(regs.xmm[x]))

int migrate(int nid, void (*callback_fn)(void *), void *callback_args)
{
	struct regset_x86_64 regs;
	int ret;

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

	asm volatile (
			"mov %0, %%edi;"
			"mov %1, %%rsi;"
			"mov %2, %%eax;"
			"syscall;"
		:
		: "g"(nid), "g"(&regs), "i"(SYSCALL_POPCORN_MIGRATE)
		: "edi", "rsi", "eax"
	);

migrated:
	asm volatile ("mfence" ::: "memory");
#ifdef WAIT_FOR_DEBUGGER
	while (__wait_for_debugger);
#endif

	SET_REGISTER(rbp);
	SET_REGISTER(rbx);
	SET_REGISTER(r12);
	SET_REGISTER(r13);
	SET_REGISTER(r14);
	SET_REGISTER(r15);

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

	asm volatile ("mov %%eax, %0" : "=g"(ret));
	if (ret < 0) return ret * -1;

	if (callback_fn) callback_fn(callback_args);
	return 0;
}

#endif
