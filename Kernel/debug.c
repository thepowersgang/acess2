/*
 * AcessOS Microkernel Version
 * debug.c
 */
#include <common.h>
#include <stdarg.h>

// === MACROS ===
#define E9(ch)	__asm__ __volatile__ ("outb %%al, $0xe9"::"a"(((Uint8)ch)))

// === IMPORTS ===
extern void Threads_Dump();

// === GLOBALS ===
 int	gDebug_Level = 0;

// === CODE ===
static void E9_Str(char *Str)
{
	while(*Str)	E9(*Str++);
}

void E9_Fmt(const char *format, va_list *args)
{
	char	c, pad = ' ';
	 int	minSize = 0;
	char	tmpBuf[34];	// For Integers
	char	*p = NULL;
	 int	isLongLong = 0;
	Uint64	arg;
  
	while((c = *format++) != 0)
	{
		// Non control character
		if( c != '%' ) {
			E9(c);
			continue;
		}
		
		c = *format++;
		
		// Literal %
		if(c == '%') {
			E9('%');
			continue;
		}
		
		// Pointer
		if(c == 'p') {
			Uint	ptr = va_arg(*args, Uint);
			E9('*');	E9('0');	E9('x');
			p = tmpBuf;
			itoa(p, ptr, 16, BITS/4, '0');
			goto printString;
		}
		
		// Get Argument
		arg = va_arg(*args, Uint);
		
		// Padding
		if(c == '0') {
			pad = '0';
			c = *format++;
		} else
			pad = ' ';
		
		// Minimum length
		minSize = 1;
		if('1' <= c && c <= '9')
		{
			minSize = 0;
			while('0' <= c && c <= '9')
			{
				minSize *= 10;
				minSize += c - '0';
				c = *format++;
			}
		}
		
		// Long (default)
		isLongLong = 0;
		if(c == 'l') {
			c = *format++;
			if(c == 'l') {
				#if BITS == 32
				arg |= va_arg(*args, Uint);
				#endif
				c = *format++;
				isLongLong = 1;
			}
		}
		
		p = tmpBuf;
		switch (c) {
		case 'd':
		case 'i':
			if( (isLongLong && arg >> 63) || (!isLongLong && arg >> 31) ) {
				E9('-');
				arg = -arg;
			}
			itoa(p, arg, 10, minSize, pad);
			goto printString;
		case 'u':
			itoa(p, arg, 10, minSize, pad);
			goto printString;
		case 'x':
			itoa(p, arg, 16, minSize, pad);
			goto printString;
		case 'o':
			itoa(p, arg, 8, minSize, pad);
			goto printString;
		case 'b':
			itoa(p, arg, 2, minSize, pad);
			goto printString;

		case 'B':	//Boolean
			if(arg)	E9_Str("True");
			else	E9_Str("False");
			break;
		
		case 's':
			p = (char*)(Uint)arg;
		printString:
			if(!p)		p = "(null)";
			while(*p)	E9(*p++);
			break;
			
		default:	E9(arg);	break;
		}
    }
}

/**
 * \fn void LogV(char *Fmt, va_list Args)
 */
void LogV(char *Fmt, va_list Args)
{
	E9_Str("Log: ");
	E9_Fmt(Fmt, &Args);
	E9('\n');
}
/**
 * \fn void LogF(char *Msg, ...)
 */
void LogF(char *Fmt, ...)
{
	va_list	args;
	
	va_start(args, Fmt);
	
	E9_Fmt(Fmt, &args);
	
	va_end(args);
}
/**
 * \fn void Log(char *Msg, ...)
 */
void Log(char *Fmt, ...)
{
	va_list	args;
	
	E9_Str("Log: ");
	va_start(args, Fmt);
	E9_Fmt(Fmt, &args);
	va_end(args);
	E9('\n');
}
void Warning(char *Fmt, ...)
{
	va_list	args;
	E9_Str("Warning: ");
	va_start(args, Fmt);
	E9_Fmt(Fmt, &args);
	va_end(args);
	E9('\n');
}
void Panic(char *Fmt, ...)
{
	va_list	args;
	E9_Str("Panic: ");
	va_start(args, Fmt);
	E9_Fmt(Fmt, &args);
	va_end(args);
	E9('\n');
	
	Threads_Dump();
	
	__asm__ __volatile__ ("xchg %bx, %bx");
	__asm__ __volatile__ ("cli;\n\thlt");
	for(;;)	__asm__ __volatile__ ("hlt");
}

void Debug_Enter(char *FuncName, char *ArgTypes, ...)
{
	va_list	args;
	 int	i = gDebug_Level ++;
	 int	pos;
	
	va_start(args, ArgTypes);
	
	while(i--)	E9(' ');
	
	E9_Str(FuncName);	E9_Str(": (");
	
	while(*ArgTypes)
	{
		pos = strpos(ArgTypes, ' ');
		if(pos != -1)	ArgTypes[pos] = '\0';
		if(pos == -1 || pos > 1) {
			E9_Str(ArgTypes+1);
			E9('=');
		}
		if(pos != -1)	ArgTypes[pos] = ' ';
		switch(*ArgTypes)
		{
		case 'p':	E9_Fmt("%p", &args);	break;
		case 's':	E9_Fmt("'%s'", &args);	break;
		case 'i':	E9_Fmt("%i", &args);	break;
		case 'u':	E9_Fmt("%u", &args);	break;
		case 'x':	E9_Fmt("0x%x", &args);	break;
		case 'b':	E9_Fmt("0b%b", &args);	break;
		// Extended (64-Bit)
		case 'X':	E9_Fmt("0x%llx", &args);	break;
		case 'B':	E9_Fmt("0b%llb", &args);	break;
		}
		if(pos != -1) {
			E9(',');	E9(' ');
		}
		
		if(pos == -1)	break;
		ArgTypes = &ArgTypes[pos+1];
	}
	
	va_end(args);
	E9(')');	E9('\n');
}

void Debug_Log(char *FuncName, char *Fmt, ...)
{
	va_list	args;
	 int	i = gDebug_Level;
	
	va_start(args, Fmt);
	
	while(i--)	E9(' ');
	
	E9_Str(FuncName);	E9_Str(": ");
	E9_Fmt(Fmt, &args);
	
	va_end(args);
	E9('\n');
}

void Debug_Leave(char *FuncName, char RetType, ...)
{
	va_list	args;
	 int	i = --gDebug_Level;
	
	va_start(args, RetType);
	
	// Indenting
	while(i--)	E9(' ');
	
	E9_Str(FuncName);	E9_Str(": RETURN");
	
	// No Return
	if(RetType == '-') {
		E9('\n');
		return;
	}
	
	E9(' ');
	switch(RetType)
	{
	case 'n':	E9_Str("NULL");	break;
	case 'p':	E9_Fmt("%p", &args);	break;
	case 's':	E9_Fmt("'%s'", &args);	break;
	case 'i':	E9_Fmt("%i", &args);	break;
	case 'u':	E9_Fmt("%u", &args);	break;
	case 'x':	E9_Fmt("0x%x", &args);	break;
	// Extended (64-Bit)
	case 'X':	E9_Fmt("0x%llx", &args);	break;
	}
	E9('\n');
	
	va_end(args);
}

void Debug_HexDump(char *Header, void *Data, Uint Length)
{
	Uint8	*cdat = Data;
	Uint	pos = 0;
	E9_Str(Header);
	LogF(" (Hexdump of %p)\n", Data);
	
	while(Length >= 16)
	{
		Log("%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			pos,
			cdat[0], cdat[1], cdat[2], cdat[3], cdat[4], cdat[5], cdat[6], cdat[7],
			cdat[8], cdat[9], cdat[10], cdat[11], cdat[12], cdat[13], cdat[14], cdat[15]
			);
		Length -= 16;
		cdat += 16;
		pos += 16;
	}
	
	LogF("Log: %04x: ", pos);
	while(Length)
	{
		Uint	byte = *cdat;
		LogF("%02x ", byte);
		Length--;
		cdat ++;
	}
	E9('\n');
}

// --- EXPORTS ---
EXPORT(Warning);
EXPORT(Debug_Enter);
EXPORT(Debug_Log);
EXPORT(Debug_Leave);
