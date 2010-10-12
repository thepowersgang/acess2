/*
 */
#include <acess.h>
#include <arch.h>

// === IMPORTS ===
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

// === Endianness ===
Uint32 BigEndian32(Uint32 Value)
{
	Uint32	ret;
	ret = (Value >> 24);
	ret |= ((Value >> 16) & 0xFF) << 8;
	ret |= ((Value >>  8) & 0xFF) << 16;
	ret |= ((Value >>  0) & 0xFF) << 24;
	return ret;
}

Uint16 BigEndian16(Uint16 Value)
{
	return	(Value>>8)|(Value<<8);
}

// === Memory Manipulation ===
int memcmp(const void *__dest, const void *__src, size_t __count)
{
	if( ((tVAddr)__dest & 7) != ((tVAddr)__src & 7) ) {
		const Uint8	*src = __src, *dst = __dest;
		while(__count)
		{
			if( *src != *dst )
				return *dst - *src;
			src ++;	dst ++;	__count --;
		}
		return 0;
	}
	else {
	        const Uint8     *src = __src;
	        const Uint8	*dst = __dest;
		const Uint64	*src64, *dst64;
		
	        while( (tVAddr)src & 7 && __count ) {
			if( *src != *dst )
				return *dst - *src;
	                dst ++;	src ++;	__count --;
	        }

		src64 = (void*)src;
		dst64 = (void*)dst;

		while( __count >= 8 )
		{
			if( *src64 != *dst64 )
			{
				src = (void*)src64;
				dst = (void*)dst64;
				if(src[0] != dst[0])	return dst[0]-src[0];
				if(src[1] != dst[1])	return dst[1]-src[1];
				if(src[2] != dst[2])	return dst[2]-src[2];
				if(src[3] != dst[3])	return dst[3]-src[3];
				if(src[4] != dst[4])	return dst[4]-src[4];
				if(src[5] != dst[5])	return dst[5]-src[5];
				if(src[6] != dst[6])	return dst[6]-src[6];
				if(src[7] != dst[7])	return dst[7]-src[7];
				return -1;	// This should never happen
			}
			__count -= 8;
			src64 ++;
			dst64 ++;
		}

	        src = (void*)src64;
	        dst = (void*)dst64;
	        while( __count-- )
		{
			if(*dst != *src)	return *dst - *src;
			dst ++;
			src ++;
		}
	}
	return 0;
}

void *memcpy(void *__dest, const void *__src, size_t __count)
{
	if( ((tVAddr)__dest & 7) != ((tVAddr)__src & 7) )
		__asm__ __volatile__ ("rep movsb" : : "D"(__dest),"S"(__src),"c"(__count));
	else {
		const Uint8	*src = __src;
		Uint8	*dst = __dest;
		while( (tVAddr)src & 7 && __count ) {
			*dst++ = *src++;
			__count --;
		}

		__asm__ __volatile__ ("rep movsq" : : "D"(dst),"S"(src),"c"(__count/8));
		src += __count & ~7;
		dst += __count & ~7;
		__count = __count & 7;
		while( __count-- )
			*dst++ = *src++;
	}
	return __dest;
}

void *memset(void *__dest, int __val, size_t __count)
{
	if( __val != 0 || ((tVAddr)__dest & 7) != 0 )
		__asm__ __volatile__ ("rep stosb" : : "D"(__dest),"a"(__val),"c"(__count));
	else {
		Uint8   *dst = __dest;

		__asm__ __volatile__ ("rep stosq" : : "D"(dst),"a"(0),"c"(__count/8));
		dst += __count & ~7;
		__count = __count & 7;
		while( __count-- )
		        *dst++ = 0;
	}
	return __dest;
}

void *memsetd(void *__dest, Uint32 __val, size_t __count)
{
	__asm__ __volatile__ ("rep stosl" : : "D"(__dest),"a"(__val),"c"(__count));
	return __dest;
}

