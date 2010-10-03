/*
 * AcessOS Microkernel Version
 * debug.c
 * 
 * TODO: Move the Debug_putchar methods out to the arch/ tree
 */
#include <acess.h>
#include <stdarg.h>

#define DEBUG_TO_E9	1
#define DEBUG_TO_SERIAL	1
#define	SERIAL_PORT	0x3F8
#define	GDB_SERIAL_PORT	0x2F8
#define	DEBUG_MAX_LINE_LEN	256

#define	LOCK_DEBUG_OUTPUT	1

// === IMPORTS ===
extern void Threads_Dump(void);
extern void	KernelPanic_SetMode(void);
extern void	KernelPanic_PutChar(char Ch);

// === PROTOTYPES ===
 int	putDebugChar(char ch);
 int	getDebugChar(void);
static void	Debug_Putchar(char ch);
static void	Debug_Puts(int DbgOnly, char *Str);
void	Debug_Fmt(const char *format, va_list args);

// === GLOBALS ===
 int	gDebug_Level = 0;
 int	giDebug_KTerm = -1;
 int	gbDebug_SerialSetup = 0;
 int	gbGDB_SerialSetup = 0;
 int	gbDebug_IsKPanic = 0;
volatile int	gbInPutChar = 0;
#if LOCK_DEBUG_OUTPUT
tShortSpinlock	glDebug_Lock;
#endif

// === CODE ===
int putDebugChar(char ch)
{
	if(!gbGDB_SerialSetup) {
		outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
		outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		outb(GDB_SERIAL_PORT + 0, 0x0C);    // Set divisor to 12 (lo byte) 9600 baud
		outb(GDB_SERIAL_PORT + 1, 0x00);    //  (base is         (hi byte)
		outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit (8N1)
		outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
		outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		gbDebug_SerialSetup = 1;
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 0x20) == 0 );
	outb(GDB_SERIAL_PORT, ch);
	return 0;
}
int getDebugChar(void)
{
	if(!gbGDB_SerialSetup) {
		outb(GDB_SERIAL_PORT + 1, 0x00);    // Disable all interrupts
		outb(GDB_SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		outb(GDB_SERIAL_PORT + 0, 0x0C);    // Set divisor to 12 (lo byte) 9600 baud
		outb(GDB_SERIAL_PORT + 1, 0x00);    //                   (hi byte)
		outb(GDB_SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
		outb(GDB_SERIAL_PORT + 2, 0xC7);    // Enable FIFO with 14-byte threshold and clear it
		outb(GDB_SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
		gbDebug_SerialSetup = 1;
	}
	while( (inb(GDB_SERIAL_PORT + 5) & 1) == 0)	;
	return inb(GDB_SERIAL_PORT);
}

static void Debug_PutCharDebug(char ch)
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
		gbDebug_SerialSetup = 1;
	}
	while( (inb(SERIAL_PORT + 5) & 0x20) == 0 );
	outb(SERIAL_PORT, ch);
	#endif
}

static void Debug_Putchar(char ch)
{	
	Debug_PutCharDebug(ch);
	if( !gbDebug_IsKPanic )
	{
		if(gbInPutChar)	return ;
		gbInPutChar = 1;
		if(giDebug_KTerm != -1)
			VFS_Write(giDebug_KTerm, 1, &ch);
		gbInPutChar = 0;
	}
	else
		KernelPanic_PutChar(ch);
}

static void Debug_Puts(int UseKTerm, char *Str)
{
	 int	len = 0;
	while( *Str )
	{
		Debug_PutCharDebug( *Str );
		
		if( gbDebug_IsKPanic )
			KernelPanic_PutChar(*Str);
		len ++;
		Str ++;
	}
	
	Str -= len;
	
	if( UseKTerm && !gbDebug_IsKPanic && giDebug_KTerm != -1)
	{
		if(gbInPutChar)	return ;
		gbInPutChar = 1;
		VFS_Write(giDebug_KTerm, len, Str);
		gbInPutChar = 0;
	}
}

void Debug_DbgOnlyFmt(const char *format, va_list args)
{
	char	buf[DEBUG_MAX_LINE_LEN];
	 int	len;
	buf[DEBUG_MAX_LINE_LEN-1] = 0;
	len = vsnprintf(buf, DEBUG_MAX_LINE_LEN-1, format, args);
	//if( len < DEBUG_MAX_LINE )
		// do something
	Debug_Puts(0, buf);
}

void Debug_Fmt(const char *format, va_list args)
{
	char	buf[DEBUG_MAX_LINE_LEN];
	 int	len;
	buf[DEBUG_MAX_LINE_LEN-1] = 0;
	len = vsnprintf(buf, DEBUG_MAX_LINE_LEN-1, format, args);
	//if( len < DEBUG_MAX_LINE )
		// do something
	Debug_Puts(1, buf);
	return ;
}

void Debug_FmtS(const char *format, ...)
{
	va_list	args;	
	va_start(args, format);
	Debug_Fmt(format, args);
	va_end(args);
}

void Debug_KernelPanic()
{
	gbDebug_IsKPanic = 1;
	KernelPanic_SetMode();
}

/**
 * \fn void LogF(char *Msg, ...)
 */
void LogF(char *Fmt, ...)
{
	va_list	args;

	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif
	
	va_start(args, Fmt);

	Debug_Fmt(Fmt, args);

	va_end(args);
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}
/**
 * \fn void Debug(char *Msg, ...)
 * \brief Print only to the debug channel
 */
void Debug(char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif

	Debug_Puts(0, "Debug: ");
	va_start(args, Fmt);
	Debug_DbgOnlyFmt(Fmt, args);
	va_end(args);
	Debug_PutCharDebug('\r');
	Debug_PutCharDebug('\n');
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}
/**
 * \fn void Log(char *Msg, ...)
 */
void Log(char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif

	Debug_Puts(1, "Log: ");
	va_start(args, Fmt);
	Debug_Fmt(Fmt, args);
	va_end(args);
	Debug_Putchar('\r');
	Debug_Putchar('\n');
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}
void Warning(char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif
	
	Debug_Puts(1, "Warning: ");
	va_start(args, Fmt);
	Debug_Fmt(Fmt, args);
	va_end(args);
	Debug_Putchar('\r');
	Debug_Putchar('\n');
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}
void Panic(char *Fmt, ...)
{
	va_list	args;
	
	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif
	// And never SHORTREL
	
	Debug_KernelPanic();
	
	Debug_Puts(1, "Panic: ");
	va_start(args, Fmt);
	Debug_Fmt(Fmt, args);
	va_end(args);
	Debug_Putchar('\r');
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
	 int	i;
	 int	pos;
	tTID	tid = Threads_GetTID();
	 
	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif

	i = gDebug_Level ++;

	va_start(args, ArgTypes);

	while(i--)	Debug_Putchar(' ');

	Debug_Puts(1, FuncName);
	Debug_FmtS("[%i]", tid);
	Debug_Puts(1, ": (");

	while(*ArgTypes)
	{
		pos = strpos(ArgTypes, ' ');
		if(pos != -1)	ArgTypes[pos] = '\0';
		if(pos == -1 || pos > 1) {
			Debug_Puts(1, ArgTypes+1);
			Debug_Putchar('=');
		}
		if(pos != -1)	ArgTypes[pos] = ' ';
		switch(*ArgTypes)
		{
		case 'p':	LogF("%p", va_arg(args, void*));	break;
		case 's':	LogF("'%s'", va_arg(args, char*));	break;
		case 'i':	LogF("%i", va_arg(args, int));	break;
		case 'u':	LogF("%u", va_arg(args, Uint));	break;
		case 'x':	LogF("0x%x", va_arg(args, Uint));	break;
		case 'b':	LogF("0b%b", va_arg(args, Uint));	break;
		case 'X':	LogF("0x%llx", va_arg(args, Uint64));	break;	// Extended (64-Bit)
		case 'B':	LogF("0b%llb", va_arg(args, Uint64));	break;	// Extended (64-Bit)
		}
		if(pos != -1) {
			Debug_Putchar(',');	Debug_Putchar(' ');
		}

		if(pos == -1)	break;
		ArgTypes = &ArgTypes[pos+1];
	}

	va_end(args);
	Debug_Putchar(')');	Debug_Putchar('\r');	Debug_Putchar('\n');
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void Debug_Log(char *FuncName, char *Fmt, ...)
{
	va_list	args;
	 int	i = gDebug_Level;
	tTID	tid = Threads_GetTID();

	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif

	va_start(args, Fmt);

	while(i--)	Debug_Putchar(' ');

	Debug_Puts(1, FuncName);
	Debug_FmtS("[%i]", tid);
	Debug_Puts(1, ": ");
	Debug_Fmt(Fmt, args);

	va_end(args);
	Debug_Putchar('\r');
	Debug_Putchar('\n');
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void Debug_Leave(char *FuncName, char RetType, ...)
{
	va_list	args;
	 int	i;
	tTID	tid = Threads_GetTID();

	#if LOCK_DEBUG_OUTPUT
	SHORTLOCK(&glDebug_Lock);
	#endif
	
	i = --gDebug_Level;

	va_start(args, RetType);

	if( i == -1 ) {
		gDebug_Level = 0;
		i = 0;
	}
	// Indenting
	while(i--)	Debug_Putchar(' ');

	Debug_Puts(1, FuncName);
	Debug_FmtS("(%i)", tid);
	Debug_Puts(1, ": RETURN");

	// No Return
	if(RetType == '-') {
		Debug_Putchar('\r');
		Debug_Putchar('\n');
		#if LOCK_DEBUG_OUTPUT
		SHORTREL(&glDebug_Lock);
		#endif
		return;
	}

	Debug_Putchar(' ');
	switch(RetType)
	{
	case 'n':	Debug_Puts(1, "NULL");	break;
	case 'p':	Debug_Fmt("%p", args);	break;
	case 's':	Debug_Fmt("'%s'", args);	break;
	case 'i':	Debug_Fmt("%i", args);	break;
	case 'u':	Debug_Fmt("%u", args);	break;
	case 'x':	Debug_Fmt("0x%x", args);	break;
	// Extended (64-Bit)
	case 'X':	Debug_Fmt("0x%llx", args);	break;
	}
	Debug_Putchar('\r');
	Debug_Putchar('\n');

	va_end(args);
	
	#if LOCK_DEBUG_OUTPUT
	SHORTREL(&glDebug_Lock);
	#endif
}

void Debug_HexDump(char *Header, void *Data, Uint Length)
{
	Uint8	*cdat = Data;
	Uint	pos = 0;
	Debug_Puts(1, Header);
	LogF(" (Hexdump of %p)\r\n", Data);

	#define	CH(n)	((' '<=cdat[(n)]&&cdat[(n)]<=0x7F) ? cdat[(n)] : '.')

	while(Length >= 16)
	{
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

	{
		 int	i ;
		LogF("Log: %04x: ", pos);
		for(i = 0; i < Length; i ++)
		{
			LogF("%02x ", cdat[i]);
		}
		for( ; i < 16; i ++)	LogF("   ");
		LogF(" ");
		for(i = 0; i < Length; i ++)
		{
			if( i == 8 )	LogF(" ");
			LogF("%c", CH(i));
		}
	}
	
	Debug_Putchar('\r');
	Debug_Putchar('\n');
}

// --- EXPORTS ---
EXPORT(Debug);
EXPORT(Log);
EXPORT(Warning);
EXPORT(Debug_Enter);
EXPORT(Debug_Log);
EXPORT(Debug_Leave);
