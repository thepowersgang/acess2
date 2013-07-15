/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * scanf.c
 * - *scanf family of functions
 */
#include "lib.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>	// toupper
#include <assert.h>	// assert() 

// Quick booleans
typedef char	BOOL;
#define TRUE	1
#define FALSE	0

// === TYPES ===
typedef void	(*printf_puts_t)(void *h, const char *s, size_t len);
enum eFPN {
	FPN_STD,
	FPN_SCI,
	FPN_SHORTEST,
};

// === PROTOTYPES ===
void	itoa(char *buf, uint64_t num, size_t base, int minLength, char pad, int bSigned);
size_t	_printf_itoa(printf_puts_t puts_cb, void *puts_h, uint64_t num,
	size_t base, int bUpper,
	int bSigned, char SignChar, int Precision,
	int PadLength, char PadChar, int bPadRight);
size_t	_printf_ftoa_hex(printf_puts_t puts_cb, void *puts_h, long double num, int Precision, int bForcePoint, int bForceSign, int bCapitals);
size_t	_printf_ftoa(printf_puts_t puts_cb, void *puts_h, long double num, size_t Base, enum eFPN Notation, int Precision, int bForcePoint, int bForceSign, int bCapitals);

// === CODE ===
/**
 * \fn EXPORT void vsnprintf(char *buf, const char *format, va_list args)
 * \brief Prints a formatted string to a buffer
 * \param buf	Pointer - Destination Buffer
 * \param format	String - Format String
 * \param args	VarArgs List - Arguments
 */
EXPORT int _vcprintf_int(printf_puts_t puts_cb, void *puts_h, const char *format, va_list args)
{
	char	tmp[65];
	 int	c, minSize, precision, len;
	size_t	pos = 0;
	char	*p;
	uint64_t	arg;
	long double	arg_f;
//	char	cPositiveChar;
	BOOL	bLongLong, bLong, bJustifyLeft, bAltForm;
	char	cNumPad, cPlus;

	#define _addchar(ch) do { \
		char _ch = ch; \
		puts_cb(puts_h, &_ch, 1); \
		pos ++; \
	} while(0)

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		// Non-control character
		if (c != '%') {
			const char	*start = format-1;
			while( (c = *format) != 0 && c != '%' )
				format ++;
			puts_cb(puts_h, start, format - start);
			pos += format - start;
			continue;
		}
		
		// Control Character
		c = *format++;
		if(c == '%') {	// Literal %
			_addchar('%');
			continue;
		}
		
		bAltForm = 0;
		cNumPad = ' ';
		bJustifyLeft = 0;
		cPlus = '\0';
		bLong = 0;
		bLongLong = 0;
		minSize = 0;
		precision = -1;
		
		// - Flags
		do
		{
			// Alternate form (0<oct>, 0x<hex>, 123.)
			if(c == '#') {
				bAltForm = 1;
			}
			// Padding with '0'
			else if(c == '0') {
				cNumPad = '0';
			}
			// Pad on left
			else if(c == '-') {
				bJustifyLeft = 1;
			}
			// Include space for positive sign
			else if(c == ' ') {
				cPlus = ' ';
			}
			// Always include sign
			else if(c == '+') {
				cPlus = '+';
			}
			else
				break;
		}
		while( (c = *format++) );
		
		// Padding length
		if( c == '*' ) {
			// Variable length
			minSize = va_arg(args, size_t);
			c = *format++;
		}
		else if('1' <= c && c <= '9')
		{
			minSize = 0;
			while('0' <= c && c <= '9')
			{
				minSize *= 10;
				minSize += c - '0';
				c = *format++;
			}
		}

		// Precision
		if(c == '.') {
			c = *format++;
			if(c == '*') {
				precision = va_arg(args, size_t);
				c = *format++;
			}
			else if('1' <= c && c <= '9')
			{
				precision = 0;
				while('0' <= c && c <= '9')
				{
					precision *= 10;
					precision += c - '0';
					c = *format++;
				}
			}
		}
	
		// Check for long long
		if(c == 'l')
		{
			bLong = 1;
			c = *format++;
			if(c == 'l') {
				bLongLong = 1;
				c = *format++;
			}
		}
		
		// Just help things along later
		p = tmp;
		
		// Get Type
		switch( c )
		{
		// Signed Integer
		case 'd':
		case 'i':
			// Get Argument
			arg = bLongLong ? va_arg(args, int64_t) : va_arg(args, int32_t);
			if( arg == 0 && precision == 0 )
				break;
			pos += _printf_itoa(puts_cb, puts_h, arg, 10, FALSE,
				TRUE, cPlus, precision, minSize, cNumPad, bJustifyLeft);
			break;
		
		// Unsigned Integer
		case 'u':
			arg = bLongLong ? va_arg(args, int64_t) : va_arg(args, int32_t);
			pos += _printf_itoa(puts_cb, puts_h, arg, 10, FALSE,
				FALSE, '\0', precision, minSize, cNumPad, bJustifyLeft);
			break;
		
		// Pointer
		case 'p':
			_addchar('*');
			_addchar('0');
			_addchar('x');
			arg = va_arg(args, intptr_t);
			pos += _printf_itoa(puts_cb, puts_h, arg, 16, FALSE,
				FALSE, '\0', sizeof(intptr_t)*2, 0,'\0',FALSE);
			break;
		// Unsigned Hexadecimal
		case 'X':
		case 'x':
			if(bAltForm) {
				_addchar('0');
				_addchar(c);
			}
			arg = bLongLong ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
			pos += _printf_itoa(puts_cb, puts_h, arg, 16, c=='X',
				FALSE, '\0', precision, minSize,cNumPad,bJustifyLeft);
			break;
		
		// Unsigned Octal
		case 'o':
			if(bAltForm) {
				_addchar('0');
			}
			arg = bLongLong ? va_arg(args, int64_t) : va_arg(args, int32_t);
			pos += _printf_itoa(puts_cb, puts_h, arg, 8, FALSE,
				FALSE, '\0', precision, minSize,cNumPad,bJustifyLeft);
			break;
		
		// Unsigned binary
		case 'b':
			if(bAltForm) {
				_addchar('0');
				_addchar('b');
			}
			arg = bLongLong ? va_arg(args, int64_t) : va_arg(args, int32_t);
			pos += _printf_itoa(puts_cb, puts_h, arg, 2, FALSE,
				FALSE, '\0', precision, minSize,cNumPad,bJustifyLeft);
			break;

		// Standard float
		case 'f':
		case 'F':
			arg_f = bLong ? va_arg(args, long double) : va_arg(args, double);
			pos += _printf_ftoa(puts_cb, puts_h, arg_f, 10, FPN_STD,
				precision, 0, bJustifyLeft, c == 'F');
			break;
		// Scientific Float
		case 'e':
		case 'E':
			arg_f = bLong ? va_arg(args, long double) : va_arg(args, double);
			pos += _printf_ftoa(puts_cb, puts_h, arg_f, 10, FPN_SCI,
				precision, 0, bJustifyLeft, c == 'E');
			break;
		// Scientific Float
		case 'g':
		case 'G':
			arg_f = bLong ? va_arg(args, long double) : va_arg(args, double);
			pos += _printf_ftoa(puts_cb, puts_h, arg_f, 10, FPN_SHORTEST,
				precision, 0, bJustifyLeft, c == 'G');
			break;
		// Hexadecimal Scientific
		case 'a':
		case 'A':
			arg_f = bLong ? va_arg(args, long double) : va_arg(args, double);
			pos += _printf_ftoa_hex(puts_cb, puts_h, arg_f, precision, 0, bJustifyLeft, c == 'A');
			break;

		// String
		case 's':
			p = va_arg(args, char*);
			if(!p)	p = "(null)";
			//_SysDebug("vsnprintf: p = '%s'", p);
			if(precision >= 0)
				len = strnlen(p, precision);
			else
				len = strlen(p);
			if(!bJustifyLeft)
				while(minSize-- > len)	_addchar(' ');
			puts_cb(puts_h, p, len); pos += len;
			if(bJustifyLeft)
				while(minSize-- > len)	_addchar(' ');
			break;

		// Unknown, just treat it as a character
		default:
			arg = va_arg(args, uint32_t);
			_addchar(arg);
			break;
		}
	}
	#undef _addchar
	
	return pos;
}

struct s_sprintf_info {
	char	*dest;
	size_t	ofs;
	size_t	maxlen;
};

void _vsnprintf_puts(void *h, const char *str, size_t len)
{
	struct s_sprintf_info	*info = h;
	while( info->ofs < info->maxlen && len -- )
		info->dest[info->ofs++] = *str++;
}

EXPORT int vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list __args)
{
	struct s_sprintf_info	info = {__s, 0, __maxlen};
	int ret;
	ret = _vcprintf_int(_vsnprintf_puts, &info, __format, __args);
	_vsnprintf_puts(&info, "", 1);
	return ret;
}

EXPORT int snprintf(char *buf, size_t maxlen, const char *format, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = vsnprintf((char*)buf, maxlen, (char*)format, args);
	va_end(args);
	return ret;
}


EXPORT int vsprintf(char * __s, const char *__format, va_list __args)
{
	return vsnprintf(__s, 0x7FFFFFFF, __format, __args);
}
EXPORT int sprintf(char *buf, const char *format, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = vsprintf((char*)buf, (char*)format, args);
	va_end(args);
	return ret;
}

void _vfprintf_puts(void *h, const char *str, size_t len)
{
	fwrite(str, len, 1, h);
}

EXPORT int vfprintf(FILE *__fp, const char *__format, va_list __args)
{
	return _vcprintf_int(_vfprintf_puts, __fp, __format, __args);
}

EXPORT int fprintf(FILE *fp, const char *format, ...)
{
	va_list	args;
	 int	ret;
	
	// Get Size
	va_start(args, format);
	ret = vfprintf(fp, (char*)format, args);
	va_end(args);
	
	return ret;
}

EXPORT int vprintf(const char *__format, va_list __args)
{
	return vfprintf(stdout, __format, __args);
}

EXPORT int printf(const char *format, ...)
{
	va_list	args;
	 int	ret;
	
	// Get final size
	va_start(args, format);
	ret = vprintf(format, args);
	va_end(args);
	
	// Return
	return ret;
}

void itoa(char *buf, uint64_t num, size_t base, int minLength, char pad, int bSigned)
{
	struct s_sprintf_info	info = {buf, 0, 1024};
	if(!buf)	return;
	_printf_itoa(_vsnprintf_puts, &info, num, base, FALSE, bSigned, '\0', 0, minLength, pad, FALSE);
	buf[info.ofs] = '\0';
}

const char cDIGITS[] = "0123456789abcdef";
const char cUDIGITS[] = "0123456789ABCDEF";
/**
 * \brief Convert an integer into a character string
 * \param buf	Destination Buffer
 * \param num	Number to convert
 * \param base	Base-n number output
 * \param minLength	Minimum length of output
 * \param pad	Padding used to ensure minLength
 * \param bSigned	Signed number output?
 */
size_t _printf_itoa(printf_puts_t puts_cb, void *puts_h, uint64_t num,
	size_t base, int bUpper,
	int bSigned, char SignChar, int Precision,
	int PadLength, char PadChar, int bPadRight)
{
	char	tmpBuf[64+1];
	 int	pos = sizeof(tmpBuf);
	size_t	ret = 0;
	 int	sign_is_neg = 0;
	const char *map = bUpper ? cUDIGITS : cDIGITS;

	if(base > 16 || base < 2) {
		return 0;
	}
	
	if(bSigned && (int64_t)num < 0)
	{
		num = -(int64_t)num;
		sign_is_neg = 1;
	}
	
	// Encode into string
	while(num > base-1) {
		tmpBuf[--pos] = map[ num % base ];
		num = (uint64_t) num / (uint64_t)base;		// Shift {number} right 1 digit
	}
	tmpBuf[--pos] = map[ num % base ];		// Most significant digit of {number}

	// TODO: This assertion may not be valid
	assert(Precision <= (int)sizeof(tmpBuf));
	Precision -= sizeof(tmpBuf)-pos + (sign_is_neg || SignChar != '\0');
	while( Precision-- > 0 )
		tmpBuf[--pos] = '0';

	// Sign	
	if(sign_is_neg)
		tmpBuf[--pos] = '-';	// Negative sign character
	else if(SignChar)
		tmpBuf[--pos] = SignChar;	// positive sign character
	else {
	}
	
	// length of number, minus the sign character
	size_t len = sizeof(tmpBuf)-pos;
	PadLength -= len;
	if( !bPadRight )
	{
		while(PadLength-- > 0)
			puts_cb(puts_h, &PadChar, 1), ret ++;
	}
	
	puts_cb(puts_h, tmpBuf+pos, len);
	ret += len;

	if( bPadRight )
	{
		while(PadLength-- > 0)
			puts_cb(puts_h, &PadChar, 1), ret ++;
	}
	
	return ret;
}

int expand_double(double num, uint64_t *Significand, int16_t *Exponent, int *SignIsNeg)
{
	// IEEE 754 binary64
	#if 0
	{
		uint64_t test_exp = 0xC000000000000000;
		double test_value = -2.0f;
		assert( *((uint64_t*)&test_value) == test_exp );
	}
	#endif
	
	const uint64_t	*bit_rep = (void*)&num;
	
	*SignIsNeg = *bit_rep >> 63;
	*Exponent = ((*bit_rep >> 52) & 0x7FF) - 1023;
	*Significand = (*bit_rep & ((1ULL << 52)-1)) << (64-52);

//	printf("%llx %i %i %llx\n", *bit_rep, (int)*SignIsNeg, (int)*Exponent, *Significand);

	// Subnormals
	if( *Exponent == -0x3FF && *Significand != 0 )
		return 1;
	// Infinity
	if( *Exponent == 0x400 && *Significand == 0)
		return 2;
	// NaNs
	if( *Exponent == 0x400 && *Significand != 0)
		return 3;

	return 0;
}

/**
 * Internal function
 * \return Remainder
 */
double _longdiv(double num, double den, int *quot)
{
	assert(num >= 0);
	assert(den > 0);
//	printf("%llu / %llu\n", (long long int)num, (long long int)den);
	
	*quot = 0;
	assert(num < den*10);
	while(num >= den)
	{
		num -= den;
		(*quot) ++;
		assert( *quot < 10 );
	}
//	printf(" %i\n", *quot);
	return num;
}

size_t _printf_ftoa_hex(printf_puts_t puts_cb, void *puts_h, long double num, int Precision, int bForcePoint, int bForceSign, int bCapitals)
{
	uint64_t	significand;
	int16_t	exponent;
	 int	signisneg;
	 int	rv;
	size_t	ret = 0;

	#define _putch(_ch) do{\
		char __ch = (bCapitals ? toupper(_ch) : _ch);\
		puts_cb(puts_h, &__ch, 1); \
		ret ++;\
	}while(0)

	if( Precision == -1 )
		Precision = (64-4)/4;

	rv = expand_double(num, &significand, &exponent, &signisneg);
	switch(rv)
	{
	// 0: No errors, nothing special
	case 0:
		break;
	// 1: Subnormals
	case 1:
		// TODO: Subnormal = 0?
		break;
	// 2: Infinity
	case 2:
		puts_cb(puts_h, "inf", 3);
		return 3;
	case 3:
		puts_cb(puts_h, "NaN", 3);
		return 3;
	}
	
	// int	exp_ofs = exponent % 4;
	// int	exp_16 = exponent - exp_ofs;
	//uint8_t	whole = (1 << exp_ofs) | ((significand >> (64-3)) >> exp_ofs);
	//significand <<= 3 - exp_ofs;
	uint8_t	whole = (rv != 1) ? 1 : 0;
	
	if( signisneg )
		_putch('-');
	else if( bForceSign )
		_putch('+');
	else {
	}
	_putch('0');
	_putch('x');
	_putch(cDIGITS[whole]);
	if( significand || bForcePoint )
		_putch('.');
	// Fractional
	while( significand && Precision -- )
	{
		uint8_t	val = significand >> (64-4);
		_putch(cDIGITS[val]);
		significand <<= 4;
	}
	_putch('p');
	//ret += _printf_itoa(puts_cb, puts_h, exp_16, 16, bCapitals, TRUE, '+', 0, 0, '\0', 0);
	ret += _printf_itoa(puts_cb, puts_h, exponent, 10, bCapitals, TRUE, '+', 0, 0, '\0', 0);
	
	#undef _putch
	return ret;
}

#if 0
size_t _printf_itoa_fixed(printf_putch_t putch_cb, void *putch_h, uint64_t num, size_t Base)
{
	uint64_t	den;
	size_t	ret = 0;

	den = 1ULL << (64-1);
	
	while( den )
	{
		putch_cb(putch_h, cDIGITS[num / den]);
		ret ++;
		num %= den;
		den /= Base;
	}

	return ret;
}

size_t _printf_ftoa_dec(printf_putch_t putch_cb, void *putch_h, long double num, enum eFPN Notation, int Precision, int bForcePoint, int bForceSign, int bCapitals)
{
	size_t	ret = 0;
	 int	i;

	#define _putch(_ch) do{\
		putch_cb(putch_h, bCapitals ? toupper(_ch) : _ch), ret++;\
	}while(0)
	
	uint64_t	significand;
	 int16_t	exponent;
	 int	signisneg;
	 int	rv = expand_double(num, &significand, &exponent, &signisneg);
	switch(rv)
	{
	// 0: No errors, nothing special
	case 0:
		break;
	// 1: Subnormals
	case 1:
		// TODO: Subnormal = 0?
		break;
	// 2: Infinity
	case 2:
		_putch('i');
		_putch('n');
		_putch('f');
		return 3;
	case 3:
		_putch('N');
		_putch('a');
		_putch('N');
		return 3;
	}

	uint64_t	whole, part;
	 int	pre_zeros, post_zeros;

	#define XXX	0
	if( Notation == FPN_SCI )
	{
	}
	else if( exponent < 0 )
	{
		whole = 0;
		part = XXX;
		pre_zeros = 0;
		post_zeros = XXX;
	}
	else if( exponent >= 64 )
	{
		part = 0;
		whole = XXX;
		pre_zeros = XXX;
		post_zeros = 0;
	}
	else
	{
		// Split into fixed point
		whole = (1 << exponent) | (significand >> (64-exponent));
		part = significand << exponent;
		pre_zeros = 0;
		post_zeros = 0;
	}

	
	// Whole portion
	ret += _printf_itoa(putch_cb, putch_h, whole, 10, FALSE, FALSE, '\0', 0, 0, '\0', FALSE);
	for(i = pre_zeros; i --; )	_putch('0');
	// TODO: Conditional point
	_putch('-');
	for(i = post_zeros; i--; )	_putch('0');
	ret += _printf_itoa_fixed(putch_cb, putch_h, part, 10);
	
	#undef _putch

	return 0;
}
#endif

size_t _printf_ftoa(printf_puts_t puts_cb, void *puts_h, long double num, size_t Base, enum eFPN Notation, int Precision, int bForcePoint, int bForceSign, int bCapitals)
{
	uint64_t	significand;
	int16_t	exponent;
	 int	signisneg;
	 int	rv, i;
	size_t	ret = 0;

	#define _putch(_ch) do{\
		char __ch = (bCapitals ? toupper(_ch) : _ch);\
		puts_cb(puts_h, &__ch, 1); \
		ret ++;\
	}while(0)

	if( Base <= 1 || Base > 16 )
		return 0;

	rv = expand_double(num, &significand, &exponent, &signisneg);
	switch(rv)
	{
	// 0: No errors, nothing special
	case 0:
		break;
	// 1: Subnormals
	case 1:
		// TODO: Subnormal = 0?
		break;
	// 2: Infinity
	case 2:
		_putch('i');
		_putch('n');
		_putch('f');
		return 3;
	case 3:
		_putch('N');
		_putch('a');
		_putch('N');
		return 3;
	}

	// - Used as 0/1 bools in arithmatic later on
	bForcePoint = !!bForcePoint;
	bForceSign = !!bForceSign;

	// Apply default to precision
	if( Precision == -1 )
		Precision = 6;

	if( num < 0 )
		num = -num;

	// Select optimum type
	if( Notation == FPN_SHORTEST )
	{
		//TODO:
		//int	first_set_sig = BSL(significand);
		// bSign+log10(num)+1+precision vs. bSign+1+1+precision+1+1+log10(exponent)
		 int	log10_num = exponent * 301 / 1000;	// log_10(2) = 0.30102999566...
		 int	log10_exp10 = 2;
		 int	sci_len = (signisneg || bForceSign) + 2 + (Precision-1) + 2 + log10_exp10;
		 int	std_whole_len = (log10_num > 0 ? log10_num : 1);
		 int	std_len = (signisneg || bForceSign) + std_whole_len + 1 + (Precision-std_whole_len);
		if( sci_len > std_len ) {
			Precision -= std_whole_len;
			Notation = FPN_STD;
		}
		else {
			Precision -= 1;
			Notation = FPN_SCI;
		}
	}

	double precision_max = 1;
	for(i = Precision; i--; )
		precision_max /= Base;

	// Determine scientific's exponent and starting denominator
	double den = 1;
	 int	sci_exponent = 0;
	if( Notation == FPN_SCI )
	{
		if( num < 1 )
		{
			while(num < 1)
			{
				num *= Base;
				sci_exponent ++;
			}
		}
		else if( num >= Base )
		{
			while(num >= Base)
			{
				num /= Base;
				sci_exponent ++;
			}
		}
		else
		{
			// no exponent
		}
		den = 1;
	}
	else if( num != 0.0 )
	{
		while( den <= num )
			den *= Base;
		den /= Base;
	}

	// Leading sign
	if( signisneg )
		_putch('-');
	else if( bForceSign )
		_putch('+');
	else {
	}
	
	num += precision_max/10 * 4.999;

	 int	value;
	// Whole section
	do
	{
		num = _longdiv(num, den, &value);
		_putch(cDIGITS[value]);
		den /= Base;
	} while( den >= 1 );

	// Decimal point (if needed/forced)	
	if( Precision > 0 || bForcePoint )
		_putch('.');
	// Decimal section
	for(i = Precision; i--; )
	{
		num = _longdiv(num, den, &value);
		_putch(cDIGITS[value]);
		den /= Base;
	}

	if( Notation == FPN_SCI )
	{
		if( Base == 16 )
			_putch('p');
		else
			_putch('e');
		ret += _printf_itoa(puts_cb, puts_h, sci_exponent, Base, FALSE, TRUE, '+', 3, 0, '\0', FALSE);
	}	

	#undef _putch

	return ret;
}
