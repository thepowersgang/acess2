/*
 * Acess-specific ACPI header.
 *
 * A modified version of the ForgeOS acforge.h, copied and modified with permision.
 */
#ifndef __ACACESS_H__
#define __ACACESS_H__

#include <acess.h>
#include <stdint.h>
#include <stdarg.h>

#define ACPI_SINGLE_THREADED

#define ACPI_USE_SYSTEM_CLIBRARY

#define ACPI_USE_DO_WHILE_0
#define ACPI_MUTEX_TYPE             ACPI_OSL_MUTEX

#define ACPI_USE_NATIVE_DIVIDE

#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1

#ifdef ARCHDIR_IS_x86
#define ACPI_MACHINE_WIDTH          32
#else
#error TODO - 64-bit support
#endif

#define ACPI_MUTEX                  void *

#define ACPI_UINTPTR_T              uintptr_t

#define ACPI_FLUSH_CPU_CACHE() __asm__ __volatile__("wbinvd");

#include "acgcc.h"

#endif /* __ACACESS_H__ */
