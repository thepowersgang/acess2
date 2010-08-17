/*
 * Acess2
 * Common Library Functions
 */
#include <acess.h>

// === CONSTANTS ===
#define	RANDOM_SEED	0xACE55052
#define	RANDOM_A	0x00731ADE
#define	RANDOM_C	12345
#define	RANDOM_SPRUCE	0xf12b02b
//                          Jan Feb Mar Apr May  Jun  Jul  Aug  Sept Oct  Nov  Dec
const short DAYS_BEFORE[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#define UNIX_TO_2K	((30*365*3600*24) + (7*3600*24))	//Normal years + leap years

// === PROTOTYPES ===
 int	atoi(const char *string);
void	itoa(char *buf, Uint num, int base, int minLength, char pad);
 int	vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args);
 int	sprintf(char *__s, const char *__format, ...);
 int	tolower(int c);
 int	strucmp(const char *Str1, const char *Str2);
 int	strpos(const char *Str, char Ch);
 Uint8	ByteSum(void *Ptr, int Size);
size_t	strlen(const char *__s);
char	*strcpy(char *__str1, const char *__str2);
char	*strncpy(char *__str1, const char *__str2, size_t max);
 int	strcmp(const char *str1, const char *str2);
 int	strncmp(const char *str1, const char *str2, size_t num);
char	*strdup(const char *Str);
char	**str_split(const char *__str, char __ch);
 int	strpos8(const char *str, Uint32 Search);
 int	ReadUTF8(Uint8 *str, Uint32 *Val);
 int	WriteUTF8(Uint8 *str, Uint32 Val);
 int	DivUp(int num, int dem);
Sint64	timestamp(int sec, int mins, int hrs, int day, int month, int year);
Uint	rand(void);
 int	CheckString(char *String);
 int	CheckMem(void *Mem, int NumBytes);
 int	ModUtil_LookupString(char **Array, char *Needle);
 int	ModUtil_SetIdent(char *Dest, char *Value);

// === EXPORTS ===
EXPORT(atoi);
EXPORT(itoa);
EXPORT(vsnprintf);
EXPORT(sprintf);
EXPORT(tolower);
EXPORT(strucmp);
EXPORT(strpos);
EXPORT(ByteSum);
EXPORT(strlen);
EXPORT(strcpy);
EXPORT(strncpy);
EXPORT(strcmp);
EXPORT(strncmp);
EXPORT(strdup);
EXPORT(str_split);
EXPORT(strpos8);
EXPORT(DivUp);
EXPORT(ReadUTF8);
EXPORT(WriteUTF8);
EXPORT(timestamp);
EXPORT(CheckString);
EXPORT(CheckMem);
EXPORT(ModUtil_LookupString);
EXPORT(ModUtil_SetIdent);

// === GLOBALS ===
static Uint	giRandomState = RANDOM_SEED;

// === CODE ===
/**
 * \brief Convert a string into an integer
 */
int atoi(const char *string)
{
	 int	ret = 0;
	 int	bNeg = 0;
	
	//Log("atoi: (string='%s')", string);
	
	// Clear non-numeric characters
	while( !('0' <= *string && *string <= '9') && *string != '-' )	string++;
	if( *string == '-' ) {
		bNeg = 1;
		while( !('0' <= *string && *string <= '9') )	string++;
	}
	
	if(*string == '0')
	{
		string ++;
		if(*string == 'x')
		{
			// Hex
			string ++;
			for( ;; string ++ )
			{
				if('0' <= *string && *string <= '9') {
					ret *= 16;
					ret += *string - '0';
				}
				else if('A' <= *string && *string <= 'F') {
					ret *= 16;
					ret += *string - 'A' + 10;
				}
				else if('a' <= *string && *string <= 'f') {
					ret *= 16;
					ret += *string - 'a' + 10;
				}
				else
					break;
			}
		}
		else	// Octal
		{
			for( ; '0' <= *string && *string <= '7'; string ++ )
			{
				ret *= 8;
				ret += *string - '0';
			}
		}
	}
	else	// Decimal
	{
		for( ; '0' <= *string && *string <= '9'; string++)
		{
			ret *= 10;
			ret += *string - '0';
		}
	}
	
	if(bNeg)	ret = -ret;
	
	//Log("atoi: RETURN %i", ret);
	
	return ret;
}

static const char cUCDIGITS[] = "0123456789ABCDEF";
/**
 * \fn void itoa(char *buf, Uint num, int base, int minLength, char pad)
 * \brief Convert an integer into a character string
 */
void itoa(char *buf, Uint num, int base, int minLength, char pad)
{
	char	tmpBuf[BITS];
	 int	pos=0, i;

	// Sanity check
	if(!buf)	return;
	
	// Sanity Check
	if(base > 16 || base < 2) {
		buf[0] = 0;
		return;
	}
	
	// Convert 
	while(num > base-1) {
		tmpBuf[pos] = cUCDIGITS[ num % base ];
		num /= (Uint)base;		// Shift `num` right 1 digit
		pos++;
	}
	tmpBuf[pos++] = cUCDIGITS[ num % base ];		// Last digit of `num`
	
	// Put in reverse
	i = 0;
	minLength -= pos;
	while(minLength-- > 0)	buf[i++] = pad;
	while(pos-- > 0)		buf[i++] = tmpBuf[pos];	// Reverse the order of characters
	buf[i] = 0;
}

/**
 * \brief Append a character the the vsnprintf output
 */
#define PUTCH(c)	do{\
	char ch=(c);\
	if(pos==__maxlen){return pos;}\
	if(__s){__s[pos++]=ch;}else{pos++;}\
	}while(0)
/**
 * \brief VArg String Number Print Formatted
 */
int vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args)
{
	char	c, pad = ' ';
	 int	minSize = 0, len;
	char	tmpBuf[34];	// For Integers
	char	*p = NULL;
	 int	isLongLong = 0;
	Uint64	val;
	size_t	pos = 0;
	// Flags
	 int	bPadLeft = 0;
	
	//Log("vsnprintf: (__s=%p, __maxlen=%i, __format='%s', ...)", __s, __maxlen, __format);
	
	while((c = *__format++) != 0)
	{
		// Non control character
		if(c != '%') { PUTCH(c); continue; }
		
		c = *__format++;
		//Log("pos = %i", pos);
		
		// Literal %
		if(c == '%') { PUTCH('%'); continue; }
		
		// Pointer - Done first for debugging
		if(c == 'p') {
			Uint	ptr = va_arg(args, Uint);
			PUTCH('*');	PUTCH('0');	PUTCH('x');
			p = tmpBuf;
			itoa(p, ptr, 16, BITS/4, '0');
			goto printString;
		}
		
		// Get Argument
		val = va_arg(args, Uint);
		
		// - Padding Side Flag
		if(c == '+') {
			bPadLeft = 1;
			c = *__format++;
		}
		
		// - Padding
		if(c == '0') {
			pad = '0';
			c = *__format++;
		}
		else
			pad = ' ';
		
		// - Minimum length
		if(c == '*') {	// Dynamic length
			minSize = val;
			val = va_arg(args, Uint);
			c = *__format++;
		}
		else if('1' <= c && c <= '9')
		{
			minSize = 0;
			while('0' <= c && c <= '9')
			{
				minSize *= 10;
				minSize += c - '0';
				c = *__format++;
			}
		}
		else
			minSize = 1;
		
		// - Default, Long or LongLong?
		isLongLong = 0;
		if(c == 'l')	// Long is actually the default on x86
		{
			c = *__format++;
			if(c == 'l') {
				#if BITS == 32
				val |= (Uint64)va_arg(args, Uint) << 32;
				#endif
				c = *__format++;
				isLongLong = 1;
			}
		}
		
		// - Now get the format code
		p = tmpBuf;
		switch(c)
		{
		case 'd':
		case 'i':
			if( isLongLong && val >> 63 ) {
				PUTCH('-');
				val = -val;
			}
			else if( !isLongLong && val >> 31 ) {
				PUTCH('-');
				val = -(Sint32)val;
			}
			itoa(p, val, 10, minSize, pad);
			goto printString;
		case 'u':
			itoa(p, val, 10, minSize, pad);
			goto printString;
		case 'x':
			itoa(p, val, 16, minSize, pad);
			goto printString;
		case 'o':
			itoa(p, val, 8, minSize, pad);
			goto printString;
		case 'b':
			itoa(p, val, 2, minSize, pad);
			goto printString;

		case 'B':	//Boolean
			if(val)	p = "True";
			else	p = "False";
			goto printString;
		
		// String - Null Terminated Array
		case 's':
			p = (char*)(tVAddr)val;
		printString:
			if(!p)		p = "(null)";
			len = strlen(p);
			if( !bPadLeft )	while(len++ < minSize)	PUTCH(pad);
			while(*p)	PUTCH(*p++);
			if( bPadLeft )	while(len++ < minSize)	PUTCH(pad);
			break;
		
		case 'C':	// Non-Null Terminated Character Array
			p = (char*)(tVAddr)val;
			if(!p)	goto printString;
			while(minSize--)	PUTCH(*p++);
			break;
		
		// Single Character
		case 'c':
		default:
			PUTCH( (Uint8)val );
			break;
		}
	}
	
	if(__s && pos != __maxlen)
		__s[pos] = '\0';
	
	return pos;
}
#undef PUTCH

/**
 */
int sprintf(char *__s, const char *__format, ...)
{
	va_list	args;
	 int	ret;
	
	va_start(args, __format);
	ret = vsnprintf(__s, -1, __format, args);
	va_end(args);
	
	return ret;
}

/**
 * \fn int tolower(int c)
 * \brief Converts a character to lower case
 */
int tolower(int c)
{
	if('A' <= c && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}

/**
 * \fn int strucmp(const char *Str1, const char *Str2)
 * \brief Compare \a Str1 and \a Str2 case-insensitively
 */
int strucmp(const char *Str1, const char *Str2)
{
	while(*Str1 && tolower(*Str1) == tolower(*Str2))
		Str1++, Str2++;
	return tolower(*Str1) - tolower(*Str2);
}

/**
 * \fn int strpos(const char *Str, char Ch)
 * \brief Search a string for an ascii character
 */
int strpos(const char *Str, char Ch)
{
	 int	pos;
	for(pos=0;Str[pos];pos++)
	{
		if(Str[pos] == Ch)	return pos;
	}
	return -1;
}

/**
 * \fn Uint8 ByteSum(void *Ptr, int Size)
 * \brief Adds the bytes in a memory region and returns the sum
 */
Uint8 ByteSum(void *Ptr, int Size)
{
	Uint8	sum = 0;
	while(Size--)	sum += *(Uint8*)Ptr++;
	return sum;
}

/**
 * \fn Uint strlen(const char *__str)
 * \brief Get the length of string
 */
Uint strlen(const char *__str)
{
	Uint	ret = 0;
	while(*__str++)	ret++;
	return ret;
}

/**
 * \fn char *strcpy(char *__str1, const char *__str2)
 * \brief Copy a string to a new location
 */
char *strcpy(char *__str1, const char *__str2)
{
	while(*__str2)
		*__str1++ = *__str2++;
	*__str1 = '\0';	// Terminate String
	return __str1;
}

/**
 * \fn char *strncpy(char *__str1, const char *__str2, size_t max)
 * \brief Copy a string to a new location
 */
char *strncpy(char *__str1, const char *__str2, size_t max)
{
	while(*__str2 && max-- >= 1)
		*__str1++ = *__str2++;
	if(max)
		*__str1 = '\0';	// Terminate String
	return __str1;
}

/**
 * \fn int strcmp(const char *str1, const char *str2)
 * \brief Compare two strings return the difference between
 *        the first non-matching characters.
 */
int strcmp(const char *str1, const char *str2)
{
	while(*str1 && *str1 == *str2)
		str1++, str2++;
	return *str1 - *str2;
}

/**
 * \fn int strncmp(const char *Str1, const char *Str2, size_t num)
 * \brief Compare strings \a Str1 and \a Str2 to a maximum of \a num characters
 */
int strncmp(const char *Str1, const char *Str2, size_t num)
{
	if(num == 0)	return 0;	// TODO: Check what should officially happen here
	while(--num && *Str1 && *Str1 == *Str2)
		Str1++, Str2++;
	return *Str1-*Str2;
}

/**
 * \fn char *strdup(const char *Str)
 * \brief Duplicates a string
 */
char *strdup(const char *Str)
{
	char	*ret;
	ret = malloc(strlen(Str)+1);
	strcpy(ret, Str);
	return ret;
}

/**
 * \brief Split a string using the passed character
 * \return NULL terminated array of strings on the heap
 * \param __str	String to split
 * \param __ch	Character to split by
 */
char **str_split(const char *__str, char __ch)
{
	 int	i, j;
	 int	len = 1;
	char	**ret;
	char	*start;
	
	for( i = 0; __str[i]; i++ )
	{
		if(__str[i] == __ch)
			len ++;
	}
	
	ret = malloc( sizeof(char*)*(len+1) + (i + 1) );
	if( !ret )	return NULL;
	
	j = 1;
	start = (char *)&ret[len+1];
	ret[0] = start;
	for( i = 0; __str[i]; i++ )
	{
		if(__str[i] == __ch) {
			*start++ = '\0';
			ret[j++] = start;
		}
		else {
			*start++ = __str[i]; 
		}
	}
	*start = '\0';
	ret[j] = NULL;
	
	return ret;
}

/**
 * \fn int DivUp(int num, int dem)
 * \brief Divide two numbers, rounding up
 * \param num	Numerator
 * \param dem	Denominator
 */
int DivUp(int num, int dem)
{
	return (num+dem-1)/dem;
}

/**
 * \fn int strpos8(const char *str, Uint32 search)
 * \brief Search a string for a UTF-8 character
 */
int strpos8(const char *str, Uint32 Search)
{
	 int	pos;
	Uint32	val = 0;
	for(pos=0;str[pos];pos++)
	{
		// ASCII Range
		if(Search < 128) {
			if(str[pos] == Search)	return pos;
			continue;
		}
		if(*(Uint8*)(str+pos) < 128)	continue;
		
		pos += ReadUTF8( (Uint8*)&str[pos], &val );
		if(val == Search)	return pos;
	}
	return -1;
}

/**
 * \fn int ReadUTF8(Uint8 *str, Uint32 *Val)
 * \brief Read a UTF-8 character from a string
 */
int ReadUTF8(Uint8 *str, Uint32 *Val)
{
	*Val = 0xFFFD;	// Assume invalid character
	
	// ASCII
	if( !(*str & 0x80) ) {
		*Val = *str;
		return 1;
	}
	
	// Middle of a sequence
	if( (*str & 0xC0) == 0x80 ) {
		return 1;
	}
	
	// Two Byte
	if( (*str & 0xE0) == 0xC0 ) {
		*Val = (*str & 0x1F) << 6;	// Upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 2;
	}
	
	// Three Byte
	if( (*str & 0xF0) == 0xE0 ) {
		*Val = (*str & 0x0F) << 12;	// Upper 4 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 3;
	}
	
	// Four Byte
	if( (*str & 0xF1) == 0xF0 ) {
		*Val = (*str & 0x07) << 18;	// Upper 3 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 12;	// Middle-upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle-lower 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 4;
}

/**
 * \fn int WriteUTF8(Uint8 *str, Uint32 Val)
 * \brief Write a UTF-8 character sequence to a string
 */
int WriteUTF8(Uint8 *str, Uint32 Val)
{
	// ASCII
	if( Val < 128 ) {
		*str = Val;
		return 1;
	}
	
	// Two Byte
	if( Val < 0x8000 ) {
		*str = 0xC0 | (Val >> 6);
		str ++;
		*str = 0x80 | (Val & 0x3F);
		return 2;
	}
	
	// Three Byte
	if( Val < 0x10000 ) {
		*str = 0xE0 | (Val >> 12);
		str ++;
		*str = 0x80 | ((Val >> 6) & 0x3F);
		str ++;
		*str = 0x80 | (Val & 0x3F);
		return 3;
	}
	
	// Four Byte
	if( Val < 0x110000 ) {
		*str = 0xF0 | (Val >> 18);
		str ++;
		*str = 0x80 | ((Val >> 12) & 0x3F);
		str ++;
		*str = 0x80 | ((Val >> 6) & 0x3F);
		str ++;
		*str = 0x80 | (Val & 0x3F);
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 0;
}

/**
 * \fn Uint64 timestamp(int sec, int mins, int hrs, int day, int month, int year)
 * \brief Converts a date into an Acess Timestamp
 */
Sint64 timestamp(int sec, int mins, int hrs, int day, int month, int year)
{
	Sint64	stamp;
	stamp = sec;
	stamp += mins*60;
	stamp += hrs*3600;
	
	stamp += day*3600*24;
	stamp += month*DAYS_BEFORE[month]*3600*24;
	if(	(
		((year&3) == 0 || year%100 != 0)
		|| (year%100 == 0 && ((year/100)&3) == 0)
		) && month > 1)	// Leap year and after feb
		stamp += 3600*24;
	
	stamp += ((365*4+1) * ((year-2000)&~3)) * 3600*24;	// Four Year Segments
	stamp += ((year-2000)&3) * 365*3600*24;	// Inside four year segment
	stamp += UNIX_TO_2K;
	
	return stamp * 1000;
}

/**
 * \fn Uint rand()
 * \brief Pseudo random number generator
 * \note Unknown effectiveness (made up on the spot)
 */
Uint rand(void)
{
	Uint	old = giRandomState;
	// Get the next state value
	giRandomState = (RANDOM_A*giRandomState + RANDOM_C) & 0xFFFFFFFF;
	// Check if it has changed, and if it hasn't, change it
	if(giRandomState == old)	giRandomState += RANDOM_SPRUCE;
	return giRandomState;
}

/// \name Memory Validation
/// \{
/**
 * \brief Checks if a string resides fully in valid memory
 */
int CheckString(char *String)
{
	if( !MM_GetPhysAddr( (tVAddr)String ) )
		return 0;
	
	// Check 1st page
	if( MM_IsUser( (tVAddr)String ) )
	{
		// Traverse String
		while( *String )
		{
			if( !MM_IsUser( (tVAddr)String ) )
				return 0;
			// Increment string pointer
			String ++;
		}
		return 1;
	}
	else if( MM_GetPhysAddr( (tVAddr)String ) )
	{
		// Traverse String
		while( *String )
		{
			if( !MM_GetPhysAddr( (tVAddr)String ) )
				return 0;
			// Increment string pointer
			String ++;
		}
		return 1;
	}
	return 0;
}

/**
 * \brief Check if a sized memory region is valid memory
 */
int CheckMem(void *Mem, int NumBytes)
{
	tVAddr	addr = (tVAddr)Mem;
	
	if( MM_IsUser( addr ) )
	{
		while( NumBytes-- )
		{
			if( !MM_IsUser( addr ) )
				return 0;
			addr ++;
		}
		return 1;
	}
	else if( MM_GetPhysAddr( addr ) )
	{
		while( NumBytes-- )
		{
			if( !MM_GetPhysAddr( addr ) )
				return 0;
			addr ++;
		}
		return 1;
	}
	return 0;
}
/// \}

/**
 * \brief Search a string array for \a Needle
 * \note Helper function for eTplDrv_IOCtl::DRV_IOCTL_LOOKUP
 */
int ModUtil_LookupString(char **Array, char *Needle)
{
	 int	i;
	if( !CheckString(Needle) )	return -1;
	for( i = 0; Array[i]; i++ )
	{
		if(strcmp(Array[i], Needle) == 0)	return i;
	}
	return -1;
}

int ModUtil_SetIdent(char *Dest, char *Value)
{
	if( !CheckMem(Dest, 32) )	return -1;
	strncpy(Dest, Value, 32);
	return 1;
}
