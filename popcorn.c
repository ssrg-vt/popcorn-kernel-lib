#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#ifdef POPCORN_X86
#define SYSCALL_SCHED_MIGRATE	330
#define SYSCALL_SCHED_PROPOSE_MIGRATION		331
#define SYSCALL_SCHED_MIGRATION_PROPOSED	332
#elif defined(POPCORN_ARM)
#define SYSCALL_SCHED_MIGRATE	285
#define SYSCALL_SCHED_PROPOSE_MIGRATION		286
#define SYSCALL_SCHED_MIGRATION_PROPOSED	287
#elif defined(POPCORN_PPC)
#define SYSCALL_SCHED_MIGRATE	285
#define SYSCALL_SCHED_PROPOSE_MIGRATION		286
#define SYSCALL_SCHED_MIGRATION_PROPOSED	287
#else
#error No architecture is specified. Check the Makefile
#endif

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

#elif defined(POPCORN_PPC)

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

void popcorn_migrate_this(int nid)
{
#ifdef POPCORN_X86
	struct regset_x86_64 regs = {};
#elif defined(POPCORN_ARM)
	struct regset_aarch64 regs = {};
#endif

	syscall(SYSCALL_SCHED_MIGRATE, nid, &regs);
}


void __static_link_bug_workaround(void)
{
	pthread_t t = { 0 };
	pthread_join(t, NULL);
}

static pthread_key_t __migrate_key;
static pthread_once_t __migrate_key_once = PTHREAD_ONCE_INIT;

static void __make_migrate_key(void)
{
	pthread_key_create(&__migrate_key, NULL);
}

struct migrate_callback {
	void (*fn)(void *);
	void *args;
	struct regset_x86_64 regs;
};

void migrate(int nid, void (*callback_fn)(void *), void *callback_args)
{
	struct regset_x86_64 *regs;
	unsigned long *rbp = __builtin_frame_address(0);
	unsigned long sp, bp;
	struct migrate_callback *cb;

	pthread_once(&__migrate_key_once, __make_migrate_key);

	if ((cb = pthread_getspecific(__migrate_key))) {
		regs = &cb->regs;

		asm volatile ("movsd %0, %%xmm0" : : "m"(regs->xmm[0]));
		asm volatile ("movsd %0, %%xmm1" : : "m"(regs->xmm[1]));
		asm volatile ("movsd %0, %%xmm2" : : "m"(regs->xmm[2]));
		asm volatile ("movsd %0, %%xmm3" : : "m"(regs->xmm[3]));
		asm volatile ("movsd %0, %%xmm4" : : "m"(regs->xmm[4]));
		asm volatile ("movsd %0, %%xmm5" : : "m"(regs->xmm[5]));
		asm volatile ("movsd %0, %%xmm6" : : "m"(regs->xmm[6]));
		asm volatile ("movsd %0, %%xmm7" : : "m"(regs->xmm[7]));
		asm volatile ("movsd %0, %%xmm8" : : "m"(regs->xmm[8]));
		asm volatile ("movsd %0, %%xmm9" : : "m"(regs->xmm[9]));
		asm volatile ("movsd %0, %%xmm10" :: "m"(regs->xmm[10]));
		asm volatile ("movsd %0, %%xmm11" :: "m"(regs->xmm[11]));
		asm volatile ("movsd %0, %%xmm12" :: "m"(regs->xmm[12]));
		asm volatile ("movsd %0, %%xmm13" :: "m"(regs->xmm[13]));
		asm volatile ("movsd %0, %%xmm14" :: "m"(regs->xmm[14]));
		asm volatile ("movsd %0, %%xmm15" :: "m"(regs->xmm[15]));

		if (cb->fn) cb->fn(cb->args);

		free(cb);
		pthread_setspecific(__migrate_key, NULL);
		return;
	}

	cb = (struct migrate_callback *)malloc(sizeof(*cb));
	cb->fn = callback_fn;
	cb->args = callback_args;
	regs = &cb->regs;

	asm volatile ("mov %%rax, %0" : "=m"(regs->rax));
	asm volatile ("mov %%rdx, %0" : "=m"(regs->rdx));
	asm volatile ("mov %%rcx, %0" : "=m"(regs->rcx));
	asm volatile ("mov %%rbx, %0" : "=m"(regs->rbx));
	// asm volatile ("mov %%rbp, %0" : "=m"(regs->rbp));
	asm volatile ("mov %%rsi, %0" : "=m"(regs->rsi));
	asm volatile ("mov %%rdi, %0" : "=m"(regs->rdi));
	// asm volatile ("mov %%rsp, %0" : "=m"(regs->rsp));
	asm volatile ("mov %%r8 , %0" : "=m"(regs->r8));
	asm volatile ("mov %%r9 , %0" : "=m"(regs->r9));
	asm volatile ("mov %%r10, %0" : "=m"(regs->r10));
	asm volatile ("mov %%r11, %0" : "=m"(regs->r11));
	asm volatile ("mov %%r12, %0" : "=m"(regs->r12));
	asm volatile ("mov %%r13, %0" : "=m"(regs->r13));
	asm volatile ("mov %%r14, %0" : "=m"(regs->r14));
	asm volatile ("mov %%r15, %0" : "=m"(regs->r15));
	// asm volatile ("movq $., %0" : "=m"(regs->rip));

	asm volatile ("pushf; pop %0" : "=m"(regs->rflags));
	asm volatile ("mov %%cs, %0" : "=m"(regs->cs));
	asm volatile ("mov %%ss, %0" : "=m"(regs->ss));
	asm volatile ("mov %%ds, %0" : "=m"(regs->ds));
	asm volatile ("mov %%es, %0" : "=m"(regs->es));
	asm volatile ("mov %%fs, %0" : "=m"(regs->fs));
	asm volatile ("mov %%gs, %0" : "=m"(regs->gs));

	asm volatile ("movsd %%xmm0, %0" : "=m"(regs->xmm[0]));
	asm volatile ("movsd %%xmm1, %0" : "=m"(regs->xmm[1]));
	asm volatile ("movsd %%xmm2, %0" : "=m"(regs->xmm[2]));
	asm volatile ("movsd %%xmm3, %0" : "=m"(regs->xmm[3]));
	asm volatile ("movsd %%xmm4, %0" : "=m"(regs->xmm[4]));
	asm volatile ("movsd %%xmm5, %0" : "=m"(regs->xmm[5]));
	asm volatile ("movsd %%xmm6, %0" : "=m"(regs->xmm[6]));
	asm volatile ("movsd %%xmm7, %0" : "=m"(regs->xmm[7]));
	asm volatile ("movsd %%xmm8, %0" : "=m"(regs->xmm[8]));
	asm volatile ("movsd %%xmm9, %0" : "=m"(regs->xmm[9]));
	asm volatile ("movsd %%xmm10, %0" : "=m"(regs->xmm[10]));
	asm volatile ("movsd %%xmm11, %0" : "=m"(regs->xmm[11]));
	asm volatile ("movsd %%xmm12, %0" : "=m"(regs->xmm[12]));
	asm volatile ("movsd %%xmm13, %0" : "=m"(regs->xmm[13]));
	asm volatile ("movsd %%xmm14, %0" : "=m"(regs->xmm[14]));
	asm volatile ("movsd %%xmm15, %0" : "=m"(regs->xmm[15]));

	regs->rip = &migrate;
	regs->rbp = bp = *rbp;
	regs->rsp = sp = (unsigned long)(rbp + 1);

	pthread_setspecific(__migrate_key, cb);

	asm volatile (
			"mov %0, %%rdi;"
			"movq %1, %%rsi;"
			"movq %2, %%rsp;"
			"movq %3, %%rbp;"
			"mov %4, %%eax;"
			"syscall;"
		:
		: "m"(nid), "m"(regs), "m"(sp), "m"(bp), "i"(SYSCALL_SCHED_MIGRATE)
		: "rdi", "rsi"
	);
}
