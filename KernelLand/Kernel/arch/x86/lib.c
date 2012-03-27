/*
 * Acess2
 *
 * arch/x86/lib.c
 * - General arch-specific stuff
 */
#include <acess.h>
#include <threads_int.h>
#include <arch_int.h>
#include <hal_proc.h>	// GetCPUNum

#define TRACE_LOCKS	0

#define DEBUG_TO_E9	1
#define DEBUG_TO_SERIAL	1
#define	SERIAL_PORT	0x3F8
#define	GDB_SERIAL_PORT	0x2F8

// === IMPRORTS ===
#if TRACE_LOCKS
extern struct sShortSpinlock	glDebug_Lock;
extern tMutex	glPhysAlloc;
#define TRACE_LOCK_COND	(Lock != &glDebug_Lock && Lock != &glThreadListLock && Lock != &glPhysAlloc.Protector)
//#define TRACE_LOCK_COND	(Lock != &glDebug_Lock && Lock != &glPhysAlloc.Protector)
#endif

// === PROTOTYPES ==
Uint64	__divmod64(Uint64 Num, Uint64 Den, Uint64 *Rem);
Uint64	__udivdi3(Uint64 Num, Uint64 Den);
Uint64	__umoddi3(Uint64 Num, Uint64 Den);

// === GLOBALS ===
 int	gbDebug_SerialSetup = 0;
 int	gbGDB_SerialSetup = 0;

// === CODE ===
/**
 * \brief Determine if a short spinlock is locked
 * \param Lock	Lock pointer
 */
int IS_LOCKED(struct sShortSpinlock *Lock)
{
	return !!Lock->Lock;
}

/**
 * \brief Check if the current CPU has the lock
 * \param Lock	Lock pointer
 */
int CPU_HAS_LOCK(struct sShortSpinlock *Lock)
{
	return Lock->Lock == GetCPUNum() + 1;
}

void __AtomicTestSetLoop(Uint *Ptr, Uint Value)
{
	__ASM__(
		"1:\n\t"
		"xor %%eax, %%eax;\n\t"
		"lock cmpxchgl %0, (%1);\n\t"
		"jnz 1b;\n\t"
		:: "r"(Value), "r"(Ptr)
		: "eax" // EAX clobbered
		);
}
/**
 * \brief Acquire a Short Spinlock
 * \param Lock	Lock pointer
 * 
 * This type of mutex should only be used for very short sections of code,
 * or in places where a Mutex_* would be overkill, such as appending
 * an element to linked list (usually two assignement lines in C)
 * 
 * \note This type of lock halts interrupts, so ensure that no timing
 * functions are called while it is held. As a matter of fact, spend as
 * little time as possible with this lock held
 * \note If \a STACKED_LOCKS is set, this type of spinlock can be nested
 */
void SHORTLOCK(struct sShortSpinlock *Lock)
{
	 int	IF;
	 int	cpu = GetCPUNum() + 1;
	
	// Save interrupt state
	__ASM__ ("pushf;\n\tpop %0" : "=r"(IF));
	IF &= 0x200;	// AND out all but the interrupt flag
	
	#if TRACE_LOCKS
	if( TRACE_LOCK_COND )
	{
		//Log_Log("LOCK", "%p locked by %p", Lock, __builtin_return_address(0));
		Debug("%i %p obtaining %p (Called by %p)", cpu-1,  __builtin_return_address(0), Lock, __builtin_return_address(1));
	}
	#endif
	
	__ASM__("cli");
	
	// Wait for another CPU to release
	__AtomicTestSetLoop( (Uint*)&Lock->Lock, cpu );
	Lock->IF = IF;
	
	#if TRACE_LOCKS
	if( TRACE_LOCK_COND )
	{
		//Log_Log("LOCK", "%p locked by %p", Lock, __builtin_return_address(0));
		Debug("%i %p locked by %p\t%p", cpu-1, Lock, __builtin_return_address(0), __builtin_return_address(1));
//		Debug("got it");
	}
	#endif
}
/**
 * \brief Release a short lock
 * \param Lock	Lock pointer
 */
void SHORTREL(struct sShortSpinlock *Lock)
{	
	#if TRACE_LOCKS
	if( TRACE_LOCK_COND )
	{
		//Log_Log("LOCK", "%p released by %p", Lock, __builtin_return_address(0));
		Debug("Lock %p released by %p\t%p", Lock, __builtin_return_address(0), __builtin_return_address(1));
	}
	#endif
	
	// Lock->IF can change anytime once Lock->Lock is zeroed
	if(Lock->IF) {
		Lock->Lock = 0;
		__ASM__ ("sti");
	}
	else {
		Lock->Lock = 0;
	}
}

// === DEBUG IO ===
#if USE_GDB_STUB
int putDebugChar(char ch)
{
	if(!gbGDB_SerialSetup) {
		outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
		outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		outb(GDB_SERIAL_PORT + 0, 0x0C);    // Set divisor to 12 (lo byte) 9600 baud
		outb(GDB_SERIAL_PORT + 1, 0x00);    //  (base is         (hi byte)
		outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit (8N1)
		outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
		outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		gbGDB_SerialSetup = 1;
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 0x20) == 0 );
	outb(GDB_SERIAL_PORT, ch);
	return 0;
}
int getDebugChar(void)
{
	if(!gbGDB_SerialSetup) {
		outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
		outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		outb(GDB_SERIAL_PORT + 0, 0x0C);    // Set divisor to 12 (lo byte) 9600 baud
		outb(GDB_SERIAL_PORT + 1, 0x00);    //                   (hi byte)
		outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
		outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
		outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		gbGDB_SerialSetup = 1;
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 1) == 0)	;
	return inb(GDB_SERIAL_PORT);
}
#endif	/* USE_GDB_STUB */

void Debug_PutCharDebug(char ch)
{
	#if DEBUG_TO_E9
	__asm__ __volatile__ ( "outb %%al, $0xe9" :: "a"(((Uint8)ch)) );
	#endif
	
	#if DEBUG_TO_SERIAL
	if(!gbDebug_SerialSetup) {
		outb(SERIAL_PORT + 1, 0x00);	// Disable all interrupts
		outb(SERIAL_PORT + 3, 0x80);	// Enable DLAB (set baud rate divisor)
		outb(SERIAL_PORT + 0, 0x01);	// Set divisor to 1 (lo byte) - 115200 baud
		outb(SERIAL_PORT + 1, 0x00);	//                  (hi byte)
		outb(SERIAL_PORT + 3, 0x03);	// 8 bits, no parity, one stop bit
		outb(SERIAL_PORT + 2, 0xC7);	// Enable FIFO with 14-byte threshold and clear it
		outb(SERIAL_PORT + 4, 0x0B);	// IRQs enabled, RTS/DSR set
		gbDebug_SerialSetup = 1;
	}
	while( (inb(SERIAL_PORT + 5) & 0x20) == 0 );
	outb(SERIAL_PORT, ch);
	#endif
}

void Debug_PutStringDebug(const char *String)
{
	while(*String)
		Debug_PutCharDebug(*String++);
}

// === IO Commands ===
void outb(Uint16 Port, Uint8 Data)
{
	__asm__ __volatile__ ("outb %%al, %%dx"::"d"(Port),"a"(Data));
}
void outw(Uint16 Port, Uint16 Data)
{
	__asm__ __volatile__ ("outw %%ax, %%dx"::"d"(Port),"a"(Data));
}
void outd(Uint16 Port, Uint32 Data)
{
	__asm__ __volatile__ ("outl %%eax, %%dx"::"d"(Port),"a"(Data));
}
Uint8 inb(Uint16 Port)
{
	Uint8	ret;
	__asm__ __volatile__ ("inb %%dx, %%al":"=a"(ret):"d"(Port));
	return ret;
}
Uint16 inw(Uint16 Port)
{
	Uint16	ret;
	__asm__ __volatile__ ("inw %%dx, %%ax":"=a"(ret):"d"(Port));
	return ret;
}
Uint32 ind(Uint16 Port)
{
	Uint32	ret;
	__asm__ __volatile__ ("inl %%dx, %%eax":"=a"(ret):"d"(Port));
	return ret;
}

/**
 * \fn void *memset(void *Dest, int Val, size_t Num)
 * \brief Do a byte granuality set of Dest
 */
void *memset(void *Dest, int Val, size_t Num)
{
	Uint32	val = Val&0xFF;
	val |= val << 8;
	val |= val << 16;
	__asm__ __volatile__ (
		"rep stosl;\n\t"
		"mov %3, %%ecx;\n\t"
		"rep stosb"
		:: "D" (Dest), "a" (val), "c" (Num/4), "r" (Num&3));
	return Dest;
}
/**
 * \brief Set double words
 */
void *memsetd(void *Dest, Uint32 Val, size_t Num)
{
	__asm__ __volatile__ ("rep stosl" :: "D" (Dest), "a" (Val), "c" (Num));
	return Dest;
}

/**
 * \fn int memcmp(const void *m1, const void *m2, size_t Num)
 * \brief Compare two pieces of memory
 */
int memcmp(const void *m1, const void *m2, size_t Num)
{
	const Uint8	*d1 = m1;
	const Uint8	*d2 = m2;
	if( Num == 0 )	return 0;	// No bytes are always identical
	
	while(Num--)
	{
		if(*d1 != *d2)
			return *d1 - *d2;
		d1 ++;
		d2 ++;
	}
	return 0;
}

/**
 * \fn void *memcpy(void *Dest, const void *Src, size_t Num)
 * \brief Copy \a Num bytes from \a Src to \a Dest
 */
void *memcpy(void *Dest, const void *Src, size_t Num)
{
	tVAddr	dst = (tVAddr)Dest;
	tVAddr	src = (tVAddr)Src;
	if( (dst & 3) != (src & 3) )
	{
		__asm__ __volatile__ ("rep movsb" :: "D" (dst), "S" (src), "c" (Num));
//		Debug("\nmemcpy:Num=0x%x by %p (UA)", Num, __builtin_return_address(0));
	}
	#if 1
	else if( Num > 128 && (dst & 15) == (src & 15) )
	{
		char	tmp[16+15];	// Note, this is a hack to save/restor xmm0
		 int	count = 16 - (dst & 15);
//		Debug("\nmemcpy:Num=0x%x by %p (SSE)", Num, __builtin_return_address(0));
		if( count < 16 )
		{
			Num -= count;
			__asm__ __volatile__ ("rep movsb" : "=D"(dst),"=S"(src): "0"(dst), "1"(src), "c"(count));
		}
		
		count = Num / 16;
		__asm__ __volatile__ (
			"movdqa 0(%5), %%xmm0;\n\t"
			"1:\n\t"
			"movdqa 0(%1), %%xmm0;\n\t"
			"movdqa %%xmm0, 0(%0);\n\t"
			"add $16,%0;\n\t"
			"add $16,%1;\n\t"
			"loop 1b;\n\t"
			"movdqa %%xmm0, 0(%5);\n\t"
			: "=r"(dst),"=r"(src)
			: "0"(dst), "1"(src), "c"(count), "r" (((tVAddr)tmp+15)&~15)
			);

		count = Num & 15;
		if(count)
			__asm__ __volatile__ ("rep movsb" :: "D"(dst), "S"(src), "c"(count));
	}
	#endif
	else
	{
//		Debug("\nmemcpy:Num=0x%x by %p", Num, __builtin_return_address(0));
		__asm__ __volatile__ (
			"rep movsl;\n\t"
			"mov %3, %%ecx;\n\t"
			"rep movsb"
			:: "D" (Dest), "S" (Src), "c" (Num/4), "r" (Num&3));
	}
	return Dest;
}

/**
 * \fn void *memcpyd(void *Dest, const void *Src, size_t Num)
 * \brief Copy \a Num DWORDs from \a Src to \a Dest
 */
void *memcpyd(void *Dest, const void *Src, size_t Num)
{
	__asm__ __volatile__ ("rep movsl" :: "D" (Dest), "S" (Src), "c" (Num));
	return Dest;
}

#include "../helpers.h"

DEF_DIVMOD(64);

Uint64 DivMod64U(Uint64 Num, Uint64 Div, Uint64 *Rem)
{
	if( Div < 0x100000000ULL && Num < 0xFFFFFFFF * Div ) {
		Uint32	rem, ret_32;
		__asm__ __volatile__(
			"div %4"
			: "=a" (ret_32), "=d" (rem)
			: "a" ( (Uint32)(Num & 0xFFFFFFFF) ), "d" ((Uint32)(Num >> 32)), "r" (Div)
			);
		if(Rem)	*Rem = rem;
		return ret_32;
	}

	return __divmod64(Num, Div, Rem);
}

/**
 * \fn Uint64 __udivdi3(Uint64 Num, Uint64 Den)
 * \brief Divide two 64-bit integers
 */
Uint64 __udivdi3(Uint64 Num, Uint64 Den)
{
	if(Den == 0) {
		__asm__ __volatile__ ("int $0x0");
		return -1;
	}
	// Common speedups
	if(Num <= 0xFFFFFFFF && Den <= 0xFFFFFFFF)
		return (Uint32)Num / (Uint32)Den;
	if(Den == 1)	return Num;
	if(Den == 2)	return Num >> 1;	// Speed Hacks
	if(Den == 4)	return Num >> 2;	// Speed Hacks
	if(Den == 8)	return Num >> 3;	// Speed Hacks
	if(Den == 16)	return Num >> 4;	// Speed Hacks
	if(Den == 32)	return Num >> 5;	// Speed Hacks
	if(Den == 1024)	return Num >> 10;	// Speed Hacks
	if(Den == 2048)	return Num >> 11;	// Speed Hacks
	if(Den == 4096)	return Num >> 12;
	if(Num < Den)	return 0;
	if(Num < Den*2)	return 1;
	if(Num == Den*2)	return 2;

	return __divmod64(Num, Den, NULL);
}

/**
 * \fn Uint64 __umoddi3(Uint64 Num, Uint64 Den)
 * \brief Get the modulus of two 64-bit integers
 */
Uint64 __umoddi3(Uint64 Num, Uint64 Den)
{
	Uint64	ret = 0;
	if(Den == 0) {
		__asm__ __volatile__ ("int $0x0");	// Call Div by Zero Error
		return -1;
	}
	if(Den == 1)	return 0;	// Speed Hacks
	if(Den == 2)	return Num & 1;	// Speed Hacks
	if(Den == 4)	return Num & 3;	// Speed Hacks
	if(Den == 8)	return Num & 7;	// Speed Hacks
	if(Den == 16)	return Num & 15;	// Speed Hacks
	if(Den == 32)	return Num & 31;	// Speed Hacks
	if(Den == 1024)	return Num & 1023;	// Speed Hacks
	if(Den == 2048)	return Num & 2047;	// Speed Hacks
	if(Den == 4096)	return Num & 4095;	// Speed Hacks
	
	if(Num >> 32 == 0 && Den >> 32 == 0)
		return (Uint32)Num % (Uint32)Den;
	
	__divmod64(Num, Den, &ret);
	return ret;
}


// --- EXPORTS ---
EXPORT(memcpy);	EXPORT(memset);
EXPORT(memcmp);
//EXPORT(memcpyw);	EXPORT(memsetw);
EXPORT(memcpyd);	EXPORT(memsetd);
EXPORT(inb);	EXPORT(inw);	EXPORT(ind);
EXPORT(outb);	EXPORT(outw);	EXPORT(outd);
EXPORT(__udivdi3);	EXPORT(__umoddi3);

EXPORT(SHORTLOCK);
EXPORT(SHORTREL);
EXPORT(IS_LOCKED);
