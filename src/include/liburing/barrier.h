/* SPDX-License-Identifier: MIT */
#ifndef LIBURING_BARRIER_H
#define LIBURING_BARRIER_H

/*
From the kernel documentation file refcount-vs-atomic.rst:

A RELEASE memory ordering guarantees that all prior loads and
stores (all po-earlier instructions) on the same CPU are completed
before the operation. It also guarantees that all po-earlier
stores on the same CPU and all propagated stores from other CPUs
must propagate to all other CPUs before the release operation
(A-cumulative property). This is implemented using
:c:func:`smp_store_release`.

An ACQUIRE memory ordering guarantees that all post loads and
stores (all po-later instructions) on the same CPU are
completed after the acquire operation. It also guarantees that all
po-later stores on the same CPU must propagate to all other CPUs
after the acquire operation executes. This is implemented using
:c:func:`smp_acquire__after_ctrl_dep`.
*/

#ifdef __cplusplus
#include <atomic>

template <typename T>
static inline void IO_URING_WRITE_ONCE(T &var, T val)
{
	std::atomic_store_explicit(reinterpret_cast<std::atomic<T> *>(&var),
				   val, std::memory_order_relaxed);
}
template <typename T>
static inline T IO_URING_READ_ONCE(const T &var)
{
	return std::atomic_load_explicit(
		reinterpret_cast<const std::atomic<T> *>(&var),
		std::memory_order_relaxed);
}

template <typename T>
static inline void io_uring_smp_store_release(T *p, T v)
{
	std::atomic_store_explicit(reinterpret_cast<std::atomic<T> *>(p), v,
				   std::memory_order_release);
}

template <typename T>
static inline T io_uring_smp_load_acquire(const T *p)
{
	return std::atomic_load_explicit(
		reinterpret_cast<const std::atomic<T> *>(p),
		std::memory_order_acquire);
}
#else

/* From tools/include/linux/compiler.h */
/* Optimization barrier */
/* The "volatile" is due to gcc bugs */
#define io_uring_barrier()	__asm__ __volatile__("": : :"memory")

/* From tools/virtio/linux/compiler.h */
#define IO_URING_WRITE_ONCE(var, val) \
	(*((volatile __typeof(val) *)(&(var))) = (val))
#define IO_URING_READ_ONCE(var) (*((volatile __typeof(var) *)(&(var))))


/* Adapted from arch/x86/include/asm/barrier.h */
#define io_uring_mb()		asm volatile("mfence" ::: "memory")
#define io_uring_rmb()		asm volatile("lfence" ::: "memory")
#define io_uring_wmb()		asm volatile("sfence" ::: "memory")
#define io_uring_smp_rmb()	io_uring_barrier()
#define io_uring_smp_wmb()	io_uring_barrier()
#if defined(__i386__)
#define io_uring_smp_mb()	asm volatile("lock; addl $0,0(%%esp)" \
					     ::: "memory", "cc")
#else
#define io_uring_smp_mb()	asm volatile("lock; addl $0,-132(%%rsp)" \
					     ::: "memory", "cc")
#endif

#define io_uring_smp_store_release(p, v)	\
do {						\
	io_uring_barrier();			\
	IO_URING_WRITE_ONCE(*(p), (v));		\
} while (0)

#define io_uring_smp_load_acquire(p)			\
({							\
	__typeof(*p) ___p1 = IO_URING_READ_ONCE(*(p));	\
	io_uring_barrier();				\
	___p1;						\
})

#endif

#endif /* defined(LIBURING_BARRIER_H) */
