/*
 */
#include <acess.h>
#include <arch.h>

// === CODE ===

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

