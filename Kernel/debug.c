/*
 * AcessOS Microkernel Version
 * debug.c
 */
#include <acess.h>
#include <stdarg.h>

#define DEBUG_TO_E9	1
#define DEBUG_TO_SERIAL	1
#define	SERIAL_PORT	0x3F8
#define	GDB_SERIAL_PORT	0x2F8

// === IMPORTS ===
extern void Threads_Dump();

// === GLOBALS ===
 int	gDebug_Level = 0;
 int	giDebug_KTerm = -1;
 int	gbDebug_SerialSetup = 0;
 int	gbGDB_SerialSetup = 0;

// === CODE ===
int putDebugChar(char ch)
{
	if(!gbGDB_SerialSetup) {
		outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
		outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		outb(GDB_SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
		outb(GDB_SERIAL_PORT + 1, 0x00);    //                  (hi byte)
		outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
		outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
		outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		gbDebug_SerialSetup = 1;
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 0x20) == 0 );
	outb(GDB_SERIAL_PORT, ch);
	return 0;
}
int getDebugChar()
{
	if(!gbGDB_SerialSetup) {
		outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
		outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		outb(GDB_SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
		outb(GDB_SERIAL_PORT + 1, 0x00);    //                  (hi byte)
		outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
		outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
		outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		gbDebug_SerialSetup = 1;
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 1) == 0)	;
	return inb(GDB_SERIAL_PORT);
}

volatile int	gbInPutChar = 0;

static void Debug_Putchar(char ch)
{
	#if DEBUG_TO_E9
	__asm__ __volatile__ ( "outb %%al, $0xe9" :: "a"(((Uint8)ch)) );
	#endif
	
	#if DEBUG_TO_SERIAL
	if(!gbDebug_SerialSetup) {
		outb(SERIAL_PORT + 1, 0x00);	// Disable all interrupts
		outb(SERIAL_PORT + 3, 0x80);	// Enable DLAB (set baud rate divisor)
		outb(SERIAL_PORT + 0, 0x03);	// Set divisor to 3 (lo byte) 38400 baud
		outb(SERIAL_PORT + 1, 0x00);	//                  (hi byte)
		outb(SERIAL_PORT + 3, 0x03);	// 8 bits, no parity, one stop bit
		outb(SERIAL_PORT + 2, 0xC7);	// Enable FIFO with 14-byte threshold and clear it
		outb(SERIAL_PORT + 4, 0x0B);	// IRQs enabled, RTS/DSR set
		gbDebug_SerialSetup = 1;
	}
	while( (inb(SERIAL_PORT + 5) & 0x20) == 0 );
	outb(SERIAL_PORT, ch);
	#endif
	
	if(gbInPutChar)	return ;
	gbInPutChar = 1;
	if(giDebug_KTerm != -1)
		VFS_Write(giDebug_KTerm, 1, &ch);
	gbInPutChar = 0;
}

static void Debug_Puts(char *Str)
{
	while(*Str)	Debug_Putchar(*Str++);
}

void Debug_Fmt(const char *format, va_list *args)
{
	char	c, pad = ' ';
	 int	minSize = 0, len;
	char	tmpBuf[34];	// For Integers
	char	*p = NULL;
	 int	isLongLong = 0;
	Uint64	arg;
	 int	bPadLeft = 0;

	while((c = *format++) != 0)
	{
		// Non control character
		if( c != '%' ) {
			Debug_Putchar(c);
			continue;
		}

		c = *format++;

		// Literal %
		if(c == '%') {
			Debug_Putchar('%');
			continue;
		}

		// Pointer
		if(c == 'p') {
			Uint	ptr = va_arg(*args, Uint);
			Debug_Putchar('*');	Debug_Putchar('0');	Debug_Putchar('x');
			p = tmpBuf;
			itoa(p, ptr, 16, BITS/4, '0');
			goto printString;
		}

		// Get Argument
		arg = va_arg(*args, Uint);

		// - Padding Side Flag
		if(c == '+') {
			bPadLeft = 1;
			c = *format++;
		}

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
				Debug_Putchar('-');
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

		printString:
			if(!p)		p = "(null)";
			while(*p)	Debug_Putchar(*p++);
			break;

		case 'B':	//Boolean
			if(arg)	Debug_Puts("True");
			else	Debug_Puts("False");
			break;

		case 's':
			p = (char*)(Uint)arg;
			if(!p)		p = "(null)";
			len = strlen(p);
			if( !bPadLeft )	while(len++ < minSize)	Debug_Putchar(pad);
			while(*p)	Debug_Putchar(*p++);
			if( bPadLeft )	while(len++ < minSize)	Debug_Putchar(pad);
			break;

		// Single Character / Array
		case 'c':
			if(minSize == 1) {
				Debug_Putchar(arg);
				break;
			}
			p = (char*)(Uint)arg;
			if(!p)	goto printString;
			while(minSize--)	Debug_Putchar(*p++);
			break;

		default:
			Debug_Putchar(arg);
			break;
		}
    }
}

/**
 * \fn void LogF(char *Msg, ...)
 */
void LogF(char *Fmt, ...)
{
	va_list	args;

	va_start(args, Fmt);

	Debug_Fmt(Fmt, &args);

	va_end(args);
}
/**
 * \fn void Log(char *Msg, ...)
 */
void Log(char *Fmt, ...)
{
	va_list	args;

	Debug_Puts("Log: ");
	va_start(args, Fmt);
	Debug_Fmt(Fmt, &args);
	va_end(args);
	Debug_Putchar('\n');
}
void Warning(char *Fmt, ...)
{
	va_list	args;
	Debug_Puts("Warning: ");
	va_start(args, Fmt);
	Debug_Fmt(Fmt, &args);
	va_end(args);
	Debug_Putchar('\n');
}
void Panic(char *Fmt, ...)
{
	va_list	args;
	Debug_Puts("Panic: ");
	va_start(args, Fmt);
	Debug_Fmt(Fmt, &args);
	va_end(args);
	Debug_Putchar('\n');

	Threads_Dump();

	__asm__ __volatile__ ("xchg %bx, %bx");
	__asm__ __volatile__ ("cli;\n\thlt");
	for(;;)	__asm__ __volatile__ ("hlt");
}

void Debug_SetKTerminal(char *File)
{
	 int	tmp;
	if(giDebug_KTerm != -1) {
		tmp = giDebug_KTerm;
		giDebug_KTerm = -1;
		VFS_Close(tmp);
	}
	tmp = VFS_Open(File, VFS_OPENFLAG_WRITE);
	Log_Log("Debug", "Opened '%s' as 0x%x", File, tmp);
	giDebug_KTerm = tmp;
	Log_Log("Debug", "Returning to %p", __builtin_return_address(0));
}

void Debug_Enter(char *FuncName, char *ArgTypes, ...)
{
	va_list	args;
	 int	i = gDebug_Level ++;
	 int	pos;

	va_start(args, ArgTypes);

	while(i--)	Debug_Putchar(' ');

	Debug_Puts(FuncName);	Debug_Puts(": (");

	while(*ArgTypes)
	{
		pos = strpos(ArgTypes, ' ');
		if(pos != -1)	ArgTypes[pos] = '\0';
		if(pos == -1 || pos > 1) {
			Debug_Puts(ArgTypes+1);
			Debug_Putchar('=');
		}
		if(pos != -1)	ArgTypes[pos] = ' ';
		switch(*ArgTypes)
		{
		case 'p':	Debug_Fmt("%p", &args);	break;
		case 's':	Debug_Fmt("'%s'", &args);	break;
		case 'i':	Debug_Fmt("%i", &args);	break;
		case 'u':	Debug_Fmt("%u", &args);	break;
		case 'x':	Debug_Fmt("0x%x", &args);	break;
		case 'b':	Debug_Fmt("0b%b", &args);	break;
		// Extended (64-Bit)
		case 'X':	Debug_Fmt("0x%llx", &args);	break;
		case 'B':	Debug_Fmt("0b%llb", &args);	break;
		}
		if(pos != -1) {
			Debug_Putchar(',');	Debug_Putchar(' ');
		}

		if(pos == -1)	break;
		ArgTypes = &ArgTypes[pos+1];
	}

	va_end(args);
	Debug_Putchar(')');	Debug_Putchar('\n');
}

void Debug_Log(char *FuncName, char *Fmt, ...)
{
	va_list	args;
	 int	i = gDebug_Level;

	va_start(args, Fmt);

	while(i--)	Debug_Putchar(' ');

	Debug_Puts(FuncName);	Debug_Puts(": ");
	Debug_Fmt(Fmt, &args);

	va_end(args);
	Debug_Putchar('\n');
}

void Debug_Leave(char *FuncName, char RetType, ...)
{
	va_list	args;
	 int	i = --gDebug_Level;

	va_start(args, RetType);

	if( i == -1 ) {
		gDebug_Level = 0;
		i = 0;
	}
	// Indenting
	while(i--)	Debug_Putchar(' ');

	Debug_Puts(FuncName);	Debug_Puts(": RETURN");

	// No Return
	if(RetType == '-') {
		Debug_Putchar('\n');
		return;
	}

	Debug_Putchar(' ');
	switch(RetType)
	{
	case 'n':	Debug_Puts("NULL");	break;
	case 'p':	Debug_Fmt("%p", &args);	break;
	case 's':	Debug_Fmt("'%s'", &args);	break;
	case 'i':	Debug_Fmt("%i", &args);	break;
	case 'u':	Debug_Fmt("%u", &args);	break;
	case 'x':	Debug_Fmt("0x%x", &args);	break;
	// Extended (64-Bit)
	case 'X':	Debug_Fmt("0x%llx", &args);	break;
	}
	Debug_Putchar('\n');

	va_end(args);
}

void Debug_HexDump(char *Header, void *Data, Uint Length)
{
	Uint8	*cdat = Data;
	Uint	pos = 0;
	Debug_Puts(Header);
	LogF(" (Hexdump of %p)\n", Data);

	while(Length >= 16)
	{
		#define	CH(n)	((' '<=cdat[(n)]&&cdat[(n)]<=0x7F) ? cdat[(n)] : '.')
		Log("%04x: %02x %02x %02x %02x %02x %02x %02x %02x"
			" %02x %02x %02x %02x %02x %02x %02x %02x"
			"  %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
			pos,
			cdat[0], cdat[1], cdat[2], cdat[3], cdat[4], cdat[5], cdat[6], cdat[7],
			cdat[8], cdat[9], cdat[10], cdat[11], cdat[12], cdat[13], cdat[14], cdat[15],
			CH(0),	CH(1),	CH(2),	CH(3),	CH(4),	CH(5),	CH(6),	CH(7),
			CH(8),	CH(9),	CH(10),	CH(11),	CH(12),	CH(13),	CH(14),	CH(15)
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
	Debug_Putchar('\n');
}

// --- EXPORTS ---
EXPORT(Log);
EXPORT(Warning);
EXPORT(Debug_Enter);
EXPORT(Debug_Log);
EXPORT(Debug_Leave);
