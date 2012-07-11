/*
 * Acess2
 * Common Library Functions
 */
#include <acess.h>

// === CONSTANTS ===
//                          Jan Feb Mar Apr May  Jun  Jul  Aug  Sept Oct  Nov  Dec
const short DAYS_BEFORE[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#define UNIX_TO_2K	((30*365*3600*24) + (7*3600*24))	//Normal years + leap years

// === PROTOTYPES ===
#if 0
 Uint8	ByteSum(void *Ptr, int Size);
char	**str_split(const char *__str, char __ch);
 int	strpos8(const char *str, Uint32 Search);
 int	ReadUTF8(Uint8 *str, Uint32 *Val);
 int	WriteUTF8(Uint8 *str, Uint32 Val);
 int	DivUp(int num, int dem);
Sint64	timestamp(int sec, int mins, int hrs, int day, int month, int year);
void	format_date(tTime TS, int *year, int *month, int *day, int *hrs, int *mins, int *sec, int *ms);
 
 int	ModUtil_LookupString(char **Array, char *Needle);
 int	ModUtil_SetIdent(char *Dest, char *Value);
 
 int	Hex(char *Dest, size_t Size, const Uint8 *SourceData);
 int	UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString);

Uint16	SwapEndian16(Uint16 Val);
Uint32	SwapEndian32(Uint16 Val);
Uint64	SwapEndian64(Uint16 Val);
#endif

// === EXPORTS ===
EXPORT(ByteSum);
EXPORT(str_split);
EXPORT(strpos8);
EXPORT(DivUp);
EXPORT(ReadUTF8);
EXPORT(WriteUTF8);
EXPORT(timestamp);
EXPORT(ModUtil_LookupString);
EXPORT(ModUtil_SetIdent);
EXPORT(Hex);
EXPORT(UnHex);
EXPORT(SwapEndian16);
EXPORT(SwapEndian32);
EXPORT(SwapEndian64);

// === CODE ===
/**
 * \fn Uint8 ByteSum(void *Ptr, int Size)
 * \brief Adds the bytes in a memory region and returns the sum
 */
Uint8 ByteSum(const void *Ptr, int Size)
{
	Uint8	sum = 0;
	const Uint8	*data = Ptr;
	while(Size--)	sum += *(data++);
	return sum;
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
int ReadUTF8(const Uint8 *str, Uint32 *Val)
{
	Uint32	outval;
	
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
		outval = (*str & 0x1F) << 6;	// Upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return 2;	// Validity check
		outval |= (*str & 0x3F);	// Lower 6 Bits
		*Val = outval;
		return 2;
	}
	
	// Three Byte
	if( (*str & 0xF0) == 0xE0 ) {
		outval = (*str & 0x0F) << 12;	// Upper 4 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return 2;	// Validity check
		outval |= (*str & 0x3F) << 6;	// Middle 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return 3;	// Validity check
		outval |= (*str & 0x3F);	// Lower 6 Bits
		*Val = outval;
		return 3;
	}
	
	// Four Byte
	if( (*str & 0xF1) == 0xF0 ) {
		outval = (*str & 0x07) << 18;	// Upper 3 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return 2;	// Validity check
		outval |= (*str & 0x3F) << 12;	// Middle-upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return 3;	// Validity check
		outval |= (*str & 0x3F) << 6;	// Middle-lower 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return 4;	// Validity check
		outval |= (*str & 0x3F);	// Lower 6 Bits
		*Val = outval;
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
		if( str ) {
			*str = Val;
		}
		return 1;
	}
	
	// Two Byte
	if( Val < 0x8000 ) {
		if( str ) {
			*str++ = 0xC0 | (Val >> 6);
			*str++ = 0x80 | (Val & 0x3F);
		}
		return 2;
	}
	
	// Three Byte
	if( Val < 0x10000 ) {
		if( str ) {
			*str++ = 0xE0 | (Val >> 12);
			*str++ = 0x80 | ((Val >> 6) & 0x3F);
			*str++ = 0x80 | (Val & 0x3F);
		}
		return 3;
	}
	
	// Four Byte
	if( Val < 0x110000 ) {
		if( str ) {
			*str++ = 0xF0 | (Val >> 18);
			*str++ = 0x80 | ((Val >> 12) & 0x3F);
			*str++ = 0x80 | ((Val >> 6) & 0x3F);
			*str++ = 0x80 | (Val & 0x3F);
		}
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 0;
}

/**
 * \fn Uint64 timestamp(int sec, int mins, int hrs, int day, int month, int year)
 * \brief Converts a date into an Acess Timestamp
 */
Sint64 timestamp(int sec, int min, int hrs, int day, int month, int year)
{
	 int	is_leap;
	Sint64	stamp;

	if( !(0 <= sec && sec < 60) )	return 0;
	if( !(0 <= min && min < 60) )	return 0;
	if( !(0 <= hrs && hrs < 24) )	return 0;
	if( !(0 <= day && day < 31) )	return 0;
	if( !(0 <= month && month < 12) )	return 0;

	stamp = DAYS_BEFORE[month] + day;

	// Every 4 years
	// - every 100 years
	// + every 400 years
	is_leap = (year % 4 == 0) - (year % 100 == 0) + (year % 400 == 0);
	ASSERT(is_leap == 0 || is_leap == 1);

	if( is_leap && month > 1 )	// Leap year and after feb
		stamp += 1;

	// Get seconds before the first of specified year
	year -= 2000;	// Base off Y2K
	// base year days + total leap year days
	stamp += year*365 + (year/400) - (year/100) + (year/4);
	
	stamp *= 3600*24;
	stamp += UNIX_TO_2K;
	stamp += sec;
	stamp += min*60;
	stamp += hrs*3600;
	
	return stamp * 1000;
}

void format_date(tTime TS, int *year, int *month, int *day, int *hrs, int *mins, int *sec, int *ms)
{
	 int	is_leap = 0, i;

	auto Sint64 DivMod64(Sint64 N, Sint64 D, Sint64 *R);
	
	Sint64 DivMod64(Sint64 N, Sint64 D, Sint64 *R)
	{
		int sign = (N < 0) != (D < 0);
		if(N < 0)	N = -N;
		if(D < 0)	D = -D;
		if(sign)
			return -DivMod64U(N, D, (Uint64*)R);
		else
			return DivMod64U(N, D, (Uint64*)R);
	}

	// Get time
	// TODO: Leap-seconds?
	{
		Sint64	rem;
		TS = DivMod64( TS, 1000, &rem );
		*ms = rem;
		TS = DivMod64( TS, 60, &rem );
		*sec = rem;
		TS = DivMod64( TS, 60, &rem );
		*mins = rem;
		TS = DivMod64( TS, 24, &rem );
		*hrs = rem;
	}

	// Adjust to Y2K
	TS -= UNIX_TO_2K/(3600*24);

	// Year (400 yr blocks) - (400/4-3) leap years
	*year = 400 * DivMod64( TS, 365*400 + (400/4-3), &TS );
	if( TS < 366 )	// First year in 400 is a leap
		is_leap = 1;
	else
	{
		// 100 yr blocks - 100/4-1 leap years
		*year += 100 * DivMod64( TS, 365*100 + (100/4-1), &TS );
		if( TS < 366 )	// First year in 100 isn't a leap
			is_leap = 0;
		else
		{
			*year += 4 * DivMod64( TS, 365*4 + 1, &TS );
			if( TS < 366 )	// First year in 4 is a leap
				is_leap = 1;
			else
			{
				*year += DivMod64( TS, 356, &TS );
			}
		}
	}
	*year += 2000;

	ASSERT(TS >= 0);
	
	*day = 0;
	// Month (if after the first of march, which is 29 Feb in a leap year)
	if( is_leap && TS > DAYS_BEFORE[2] ) {
		TS -= 1;	// Shifts 29 Feb to 28 Feb
		*day = 1;
	}
	// Get what month it is
	for( i = 0; i < 12; i ++ ) {
		if( TS < DAYS_BEFORE[i] )
			break;
	}
	*month = i - 1;
	// Get day
	TS -= DAYS_BEFORE[i-1];
	*day += TS;	// Plus offset from leap handling above
}

/**
 * \brief Search a string array for \a Needle
 * \note Helper function for eTplDrv_IOCtl::DRV_IOCTL_LOOKUP
 */
int ModUtil_LookupString(const char **Array, const char *Needle)
{
	 int	i;
	if( !CheckString(Needle) )	return -1;
	for( i = 0; Array[i]; i++ )
	{
		if(strcmp(Array[i], Needle) == 0)	return i;
	}
	return -1;
}

int ModUtil_SetIdent(char *Dest, const char *Value)
{
	if( !CheckMem(Dest, 32) )	return -1;
	strncpy(Dest, Value, 32);
	return 1;
}

int Hex(char *Dest, size_t Size, const Uint8 *SourceData)
{
	 int	i;
	for( i = 0; i < Size; i ++ )
	{
		sprintf(Dest + i*2, "%02x", SourceData[i]);
	}
	return i*2;
}

/**
 * \brief Convert a string of hexadecimal digits into a byte stream
 */
int UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString)
{
	 int	i;
	
	for( i = 0; i < DestSize*2; i += 2 )
	{
		Uint8	val = 0;
		
		if(SourceString[i] == '\0')	break;
		
		if('0' <= SourceString[i] && SourceString[i] <= '9')
			val |= (SourceString[i]-'0') << 4;
		else if('A' <= SourceString[i] && SourceString[i] <= 'F')
			val |= (SourceString[i]-'A'+10) << 4;
		else if('a' <= SourceString[i] && SourceString[i] <= 'f')
			val |= (SourceString[i]-'a'+10) << 4;
			
		if(SourceString[i+1] == '\0')	break;
		
		if('0' <= SourceString[i+1] && SourceString[i+1] <= '9')
			val |= (SourceString[i+1] - '0');
		else if('A' <= SourceString[i+1] && SourceString[i+1] <= 'F')
			val |= (SourceString[i+1] - 'A' + 10);
		else if('a' <= SourceString[i+1] && SourceString[i+1] <= 'f')
			val |= (SourceString[i+1] - 'a' + 10);
		
		Dest[i/2] = val;
	}
	return i/2;
}

Uint16 SwapEndian16(Uint16 Val)
{
	return ((Val&0xFF)<<8) | ((Val>>8)&0xFF);
}
Uint32 SwapEndian32(Uint32 Val)
{
	return ((Val&0xFF)<<24) | ((Val&0xFF00)<<8) | ((Val>>8)&0xFF00) | ((Val>>24)&0xFF);
}
Uint64 SwapEndian64(Uint64 Val)
{
	return SwapEndian32(Val >> 32) | ((Uint64)SwapEndian32(Val) << 32);
}

