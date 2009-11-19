/*
 * Acess2
 * Common Library Functions
 */
#include <common.h>

// === CONSTANTS ===
#define	RANDOM_SEED	0xACE55052
#define	RANDOM_A	0x00731ADE
#define	RANDOM_C	12345
#define	RANDOM_SPRUCE	0xf12b02b
//                          Jan Feb Mar Apr May  Jun  Jul  Aug  Sept Oct  Nov  Dec
const short DAYS_BEFORE[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#define UNIX_TO_2K	((30*365*3600*24) + (7*3600*24))	//Normal years + leap years

// === PROTOTYPES ===
 int	ReadUTF8(Uint8 *str, Uint32 *Val);

// === GLOBALS ===
static Uint	giRandomState = RANDOM_SEED;

// === CODE ===
static const char cUCDIGITS[] = "0123456789ABCDEF";
/**
 * \fn static void itoa(char *buf, Uint num, int base, int minLength, char pad)
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
 * \fn int strucmp(char *Str1, char *Str2)
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
 * \fn int ByteSum(void *Ptr, int Size)
 * \brief Adds the bytes in a memory region and returns the sum
 */
int ByteSum(void *Ptr, int Size)
{
	 int	sum = 0;
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
 * \fn char *strcpy(const char *__str1, const char *__str2)
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
	
	stamp += ((365*4+1) * ((year-2000)&~3)) * 3600*24;	// Foour Year Segments
	stamp += ((year-2000)&3) * 365*3600*24;	// Inside four year segment
	stamp += UNIX_TO_2K;
	
	return stamp * 1000;
}

/**
 * \fn Uint rand()
 * \brief Pseudo random number generator
 * \note Unknown effectiveness (made up on the spot)
 */
Uint rand()
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
int LookupString(char **Array, char *Needle)
{
	 int	i;
	for( i = 0; Array[i]; i++ )
	{
		if(strcmp(Array[i], Needle) == 0)	return i;
	}
	return -1;
}

EXPORT(strlen);
EXPORT(strdup);
EXPORT(strcmp);
EXPORT(strncmp);
EXPORT(strcpy);
//EXPORT(strncpy);

EXPORT(timestamp);
EXPORT(ReadUTF8);
EXPORT(CheckMem);
EXPORT(CheckString);
EXPORT(LookupString);
