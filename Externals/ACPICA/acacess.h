/******************************************************************************
 *
 * Name: aclinux.h - OS specific defines, etc. for Linux
 *
 *****************************************************************************/

#ifndef _ACPICA__ACACESS_H_
#define _ACPICA__ACACESS_H_

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_MUTEX_TYPE             ACPI_OSL_MUTEX

//#define ACPI_DEBUG_OUTPUT	1

#ifdef __KERNEL__

#include <acess.h>

/* Host-dependent types and defines for in-kernel ACPICA */

#define ACPI_MACHINE_WIDTH          BITS

typedef struct sACPICache	tACPICache;

#define ACPI_CACHE_T	tACPICache
#define ACPI_SPINLOCK               tShortSpinlock*
#define ACPI_CPU_FLAGS              unsigned long

#define COMPILER_DEPENDENT_UINT64	Uint64
#define COMPILER_DEPENDENT_INT64	Sint64

#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32) do { \
	Uint64	rem; \
	Sint64	num = ((Sint64)n_hi<<32)|n_lo; \
	 int	sgn = 1; \
	if(num < 0) {num = -num; sgn = -sgn; } \
	if(d32 < 0) {d32 = -d32; sgn = -sgn; } \
	q32 = sgn * DivMod64U( num, d32, &rem ); \
	r32 = rem; \
	}while(0)
#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) do { \
	n_lo >>= 1; \
	if(n_hi & 1)	n_lo |= (1 << 31); \
	n_hi >>= 1; \
	}while(0)

#else /* !__KERNEL__ */

#error "Kernel only"

#endif /* __KERNEL__ */

/* Linux uses GCC */

#include "acgcc.h"


#if 0
#ifdef __KERNEL__
#define ACPI_SYSTEM_XFACE
#include <actypes.h>
/*
 * Overrides for in-kernel ACPICA
 */
static inline acpi_thread_id acpi_os_get_thread_id(void)
{
    return (ACPI_THREAD_ID) (unsigned long) current;
}

/*
 * The irqs_disabled() check is for resume from RAM.
 * Interrupts are off during resume, just like they are for boot.
 * However, boot has  (system_state != SYSTEM_RUNNING)
 * to quiet __might_sleep() in kmalloc() and resume does not.
 */
static inline void *acpi_os_allocate(acpi_size size)
{
    return malloc(size);
}

static inline void *acpi_os_allocate_zeroed(acpi_size size)
{
    return calloc(size, 1);
}

static inline void *acpi_os_acquire_object(acpi_cache_t * cache)
{
//    return kmem_cache_zalloc(cache,
//        irqs_disabled() ? GFP_ATOMIC : GFP_KERNEL);
}

#define ACPI_ALLOCATE(a)        acpi_os_allocate(a)
#define ACPI_ALLOCATE_ZEROED(a) acpi_os_allocate_zeroed(a)
#define ACPI_FREE(a)            free(a)

#ifndef CONFIG_PREEMPT
/*
 * Used within ACPICA to show where it is safe to preempt execution
 * when CONFIG_PREEMPT=n
 */
#define ACPI_PREEMPTION_POINT() \
    do { \
        Threads_Yield(); \
    } while (0)
#endif

/*
 * When lockdep is enabled, the spin_lock_init() macro stringifies it's
 * argument and uses that as a name for the lock in debugging.
 * By executing spin_lock_init() in a macro the key changes from "lock" for
 * all locks to the name of the argument of acpi_os_create_lock(), which
 * prevents lockdep from reporting false positives for ACPICA locks.
 */
#define AcpiOsCreateLock(__handle)				\
({								\
	tShortlock *lock = ACPI_ALLOCATE_ZEROED(sizeof(*lock));	\
								\
	if (lock) {						\
		*(__handle) = lock;				\
	}							\
	lock ? AE_OK : AE_NO_MEMORY;				\
})

#endif /* __KERNEL__ */
#endif

#endif /* __ACLINUX_H__ */
