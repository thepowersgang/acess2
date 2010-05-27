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

