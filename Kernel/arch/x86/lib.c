/*
 * AcessOS Microkernel Version
 * lib.c
 */
#include <acess.h>
#include <threads.h>

extern int	GetCPUNum(void);

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
	#if STACKED_LOCKS == 1
	return Lock->Lock == GetCPUNum() + 1;
	#elif STACKED_LOCKS == 2
	return Lock->Lock == Proc_GetCurThread();
	#else
	return 0;
	#endif
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
	 int	v = 1;
	#if LOCK_DISABLE_INTS
	 int	IF;
	#endif
	#if STACKED_LOCKS == 1
	 int	cpu = GetCPUNum() + 1;
	#elif STACKED_LOCKS == 2
	void	*thread = Proc_GetCurThread();
	#endif
	
	#if LOCK_DISABLE_INTS
	// Save interrupt state and clear interrupts
	__ASM__ ("pushf;\n\tpop %%eax\n\tcli" : "=a"(IF));
	IF &= 0x200;	// AND out all but the interrupt flag
	#endif
	
	#if STACKED_LOCKS == 1
	if( Lock->Lock == cpu ) {
		Lock->Depth ++;
		return ;
	}
	#elif STACKED_LOCKS == 2
	if( Lock->Lock == thread ) {
		Lock->Depth ++;
		return ;
	}
	#endif
	
	// Wait for another CPU to release
	while(v) {
		// CMPXCHG:
		//  If r/m32 == EAX, set ZF and set r/m32 = r32
		//  Else, clear ZF and set EAX = r/m32
		#if STACKED_LOCKS == 1
		__ASM__("lock cmpxchgl %2, (%3)"
			: "=a"(v)
			: "a"(0), "r"(cpu), "r"(&Lock->Lock)
			);
		#elif STACKED_LOCKS == 2
		__ASM__("lock cmpxchgl %2, (%3)"
			: "=a"(v)
			: "a"(0), "r"(thread), "r"(&Lock->Lock)
			);
		#else
		__ASM__("xchgl %%eax, (%%edi)":"=a"(v):"a"(1),"D"(&Lock->Lock));
		#endif
	}
	
	#if LOCK_DISABLE_INTS
	Lock->IF = IF;
	#endif
}
/**
 * \brief Release a short lock
 * \param Lock	Lock pointer
 */
void SHORTREL(struct sShortSpinlock *Lock)
{
	#if STACKED_LOCKS
	if( Lock->Depth ) {
		Lock->Depth --;
		return ;
	}
	#endif
	
	#if LOCK_DISABLE_INTS
	// Lock->IF can change anytime once Lock->Lock is zeroed
	if(Lock->IF) {
		Lock->Lock = 0;
		__ASM__ ("sti");
	}
	else {
		Lock->Lock = 0;
	}
	#else
	Lock->Lock = 0;
	#endif
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
	while(Num--)
	{
		if(*(Uint8*)m1 != *(Uint8*)m2)	break;
		m1 ++;
		m2 ++;
	}
	return *(Uint8*)m1 - *(Uint8*)m2;
}

/**
 * \fn void *memcpy(void *Dest, const void *Src, size_t Num)
 * \brief Copy \a Num bytes from \a Src to \a Dest
 */
void *memcpy(void *Dest, const void *Src, size_t Num)
{
	if( ((Uint)Dest & 3) || ((Uint)Src & 3) )
		__asm__ __volatile__ ("rep movsb" :: "D" (Dest), "S" (Src), "c" (Num));
	else {
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

/**
 * \fn Uint64 __udivdi3(Uint64 Num, Uint64 Den)
 * \brief Divide two 64-bit integers
 */
Uint64 __udivdi3(Uint64 Num, Uint64 Den)
{
	Uint64	P[2];
	Uint64	q = 0;
	 int	i;
	
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");
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
	
	// Restoring division, from wikipedia
	// http://en.wikipedia.org/wiki/Division_(digital)
	P[0] = Num;	P[1] = 0;
	for( i = 64; i--; )
	{
		// P <<= 1;
		P[1] = (P[1] << 1) | (P[0] >> 63);
		P[0] = P[0] << 1;
		
		// P -= Den << 64
		P[1] -= Den;
		
		// P >= 0
		if( !(P[1] & (1ULL<<63)) ) {
			q |= (Uint64)1 << (63-i);
		}
		else {
			//q |= 0 << (63-i);
			P[1] += Den;
		}
	}
	
	return q;
}

/**
 * \fn Uint64 __umoddi3(Uint64 Num, Uint64 Den)
 * \brief Get the modulus of two 64-bit integers
 */
Uint64 __umoddi3(Uint64 Num, Uint64 Den)
{
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");	// Call Div by Zero Error
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
	
	return Num - __udivdi3(Num, Den) * Den;
}

Uint16 LittleEndian16(Uint16 Val)
{
	return Val;
}
Uint16 BigEndian16(Uint16 Val)
{
	return ((Val&0xFF)<<8) | ((Val>>8)&0xFF);
}
Uint32 LittleEndian32(Uint32 Val)
{
	return Val;
}
Uint32 BigEndian32(Uint32 Val)
{
	return ((Val&0xFF)<<24) | ((Val&0xFF00)<<8) | ((Val>>8)&0xFF00) | ((Val>>24)&0xFF);
}

// --- EXPORTS ---
EXPORT(memcpy);	EXPORT(memset);
EXPORT(memcmp);
//EXPORT(memcpyw);	EXPORT(memsetw);
EXPORT(memcpyd);	EXPORT(memsetd);
EXPORT(inb);	EXPORT(inw);	EXPORT(ind);
EXPORT(outb);	EXPORT(outw);	EXPORT(outd);
EXPORT(__udivdi3);	EXPORT(__umoddi3);

EXPORT(LittleEndian16);	EXPORT(BigEndian16);
EXPORT(LittleEndian32);	EXPORT(BigEndian32);

EXPORT(SHORTLOCK);
EXPORT(SHORTREL);
EXPORT(IS_LOCKED);
