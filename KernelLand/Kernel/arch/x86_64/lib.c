/*
 */
#include <acess.h>
#include <arch.h>
#include <drv_serial.h>

#define DEBUG_TO_E9	1
#define DEBUG_TO_SERIAL	1
#define	SERIAL_PORT	0x3F8
#define	GDB_SERIAL_PORT	0x2F8


// === IMPORTS ===
extern int	GetCPUNum(void);
extern void	*Proc_GetCurThread(void);

// === GLOBALS ===
 int	gbDebug_SerialSetup = 0;
 int	gbGDB_SerialSetup = 0;

// === PROTOTYPEs ===
 int	putDebugChar(char ch);

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
	__ASM__ ("pushf;\n\tpop %0" : "=r"(IF));
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
		__ASM__("lock cmpxchgq %2, (%3)"
			: "=a"(v)
			: "a"(0), "r"(thread), "r"(&Lock->Lock)
			);
		#else
		__ASM__("xchgl %0, (%2)":"=a"(v):"a"(1),"D"(&Lock->Lock));
		#endif
		
		#if LOCK_DISABLE_INTS
		if( v )	__ASM__("sti");	// Re-enable interrupts
		#endif
	}
	
	#if LOCK_DISABLE_INTS
	__ASM__("cli");
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

// === DEBUG IO ===
#if USE_GDB_STUB
void initGdbSerial(void)
{
	outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
	outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(GDB_SERIAL_PORT + 0, 0x0C);    // Set divisor to 12 (lo byte) 9600 baud
	outb(GDB_SERIAL_PORT + 1, 0x00);    //  (base is         (hi byte)
	outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit (8N1)
	outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
	outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	gbDebug_SerialSetup = 1;
}
int putDebugChar(char ch)
{
	if(!gbGDB_SerialSetup) {
		initGdbSerial();
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 0x20) == 0 );
	outb(GDB_SERIAL_PORT, ch);
	return 0;
}
int getDebugChar(void)
{
	if(!gbGDB_SerialSetup) {
		initGdbSerial();
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 1) == 0)	;
	return inb(GDB_SERIAL_PORT);
}
#endif

void Debug_SerialIRQHandler(int irq, void *unused)
{
	if( (inb(SERIAL_PORT+5) & 0x01) == 0 ) {
		Debug("Serial no data");
		return ;
	}
	
	char ch = inb(SERIAL_PORT);
	//Debug("Serial RX 0x%x", ch);
	Serial_ByteReceived(gSerial_KernelDebugPort, ch);
}

void Debug_PutCharDebug(char ch)
{
	#if DEBUG_TO_E9
	__asm__ __volatile__ ( "outb %%al, $0xe9" :: "a"(((Uint8)ch)) );
	#endif
	
	#if DEBUG_TO_SERIAL
	if(!gbDebug_SerialSetup) {
		outb(SERIAL_PORT + 1, 0x00);	// Disable all interrupts
		outb(SERIAL_PORT + 3, 0x80);	// Enable DLAB (set baud rate divisor)
		outb(SERIAL_PORT + 0, 0x0C);	// Set divisor to 12 (lo byte) 9600 baud
		outb(SERIAL_PORT + 1, 0x00);	//                   (hi byte)
		outb(SERIAL_PORT + 3, 0x03);	// 8 bits, no parity, one stop bit
		outb(SERIAL_PORT + 2, 0xC7);	// Enable FIFO with 14-byte threshold and clear it
		outb(SERIAL_PORT + 4, 0x0B);	// IRQs enabled, RTS/DSR set
		outb(SERIAL_PORT + 1, 0x05);	// Enable ERBFI (Rx Full), ELSI (Line Status)
		gbDebug_SerialSetup = 1;
		IRQ_AddHandler(4, Debug_SerialIRQHandler, NULL);
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

// === PORT IO ===
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
/*
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
*/

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
	tVAddr	dst = (tVAddr)__dest, src = (tVAddr)__src;
	if( (dst & 7) != (src & 7) )
	{
		__asm__ __volatile__ ("rep movsb" : : "D"(dst),"S"(src),"c"(__count));
	}
	else
	{
		while( (src & 7) && __count ) {
			*(char*)dst++ = *(char*)src++;
			__count --;
		}

		__asm__ __volatile__ ("rep movsq" : "=D"(dst),"=S"(src) : "0"(dst),"1"(src),"c"(__count/8));
		__count = __count & 7;
		while( __count-- )
			*(char*)dst++ = *(char*)src++;
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

Uint64 DivMod64U(Uint64 Num, Uint64 Den, Uint64 *Rem)
{
	Uint64	ret, rem;
	__asm__ __volatile__(
		"div %4"
		: "=a" (ret), "=d" (rem)
		: "a" ( Num ), "d" (0), "r" (Den)
		);
	if(Rem)	*Rem = rem;
	return ret;
}

EXPORT(memcpy);
EXPORT(memset);

