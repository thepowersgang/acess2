/*
 * Acess 2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * logging.c - Kernel Logging Service
 */
#include <acess.h>

#define PRINT_ON_APPEND	1

// === CONSTANTS ===
enum eLogLevels
{
	LOG_LEVEL_KPANIC,
	LOG_LEVEL_PANIC,
	LOG_LEVEL_FATAL,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_LOG,
	LOG_LEVEL_DEBUG,
	NUM_LOG_LEVELS
};
const char	csaLevelCodes[] = {'k','p','f','e','w','n','l','d'};

// === TYPES ===
typedef struct sLogEntry
{
	struct sLogEntry	*Next;
	struct sLogEntry	*LevelNext;
	Sint64	Time;
	Uint64	Ident;
	 int	Level;
	 int	Length;
	char	Data[];
}	tLogEntry;
typedef struct sLogList
{
	tLogEntry	*Head;
	tLogEntry	*Tail;
}	tLogList;

// === PROTOTYPES ===
void	Log_AddEvent(char *Ident, int Level, char *Format, va_list Args);
static void	Log_Int_PrintMessage(tLogEntry *Entry);
void	Log_KernelPanic(char *Ident, char *Message, ...);
void	Log_Panic(char *Ident, char *Message, ...);
void	Log_Error(char *Ident, char *Message, ...);
void	Log_Warning(char *Ident, char *Message, ...);
void	Log_Log(char *Ident, char *Message, ...);
void	Log_Notice(char *Ident, char *Message, ...);
void	Log_Debug(char *Ident, char *Message, ...);
static Uint64	Log_Int_GetIdent(const char *Str);

// === GLOBALS ===
tSpinlock	glLog;
tLogList	gLog;
tLogList	gLog_Levels[NUM_LOG_LEVELS];

// === CODE ===
/**
 * \brief Adds an event to the log
 */
void Log_AddEvent(char *Ident, int Level, char *Format, va_list Args)
{
	 int	len;
	tLogEntry	*ent;
	Uint64	ident = Log_Int_GetIdent(Ident);
	
	if( Level >= NUM_LOG_LEVELS )	return;
	
	len = vsnprintf(NULL, 256, Format, Args);
	
	ent = malloc(sizeof(tLogEntry)+len+1);
	ent->Time = now();
	ent->Ident = ident;
	ent->Level = Level;
	ent->Length = len;
	vsnprintf( ent->Data, 256, Format, Args );
	
	LOCK( &glLog );
	
	ent->Next = gLog.Tail;
	if(gLog.Head)
		gLog.Tail = ent;
	else
		gLog.Tail = gLog.Head = ent;
	
	ent->LevelNext = gLog_Levels[Level].Tail;
	if(gLog_Levels[Level].Head)
		gLog_Levels[Level].Tail = ent;
	else
		gLog_Levels[Level].Tail = gLog_Levels[Level].Head = ent;
	
	RELEASE( &glLog );
	
	#if PRINT_ON_APPEND
	Log_Int_PrintMessage( ent );
	#endif
	
}

/**
 * \brief Prints a log message to the debug console
 */
void Log_Int_PrintMessage(tLogEntry *Entry)
{
	LogF("%018%c [%8s] %s\n",
		Entry->Time,
		csaLevelCodes[Entry->Level],
		&Entry->Ident,
		Entry->Data
		);
}

/**
 * \brief KERNEL PANIC!!!!
 */
void Log_KernelPanic(char *Ident, char *Message, ...)
{
	va_list	args;	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_KPANIC, Message, args);
	va_end(args);
}

/**
 * \brief Panic Message - Driver Unrecoverable error
 */
void Log_Panic(char *Ident, char *Message, ...)
{
	va_list	args;	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_PANIC, Message, args);
	va_end(args);
}

/**
 * \brief Error Message - Recoverable Error
 */
void Log_Error(char *Ident, char *Message, ...)
{
	va_list	args;	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_ERROR, Message, args);
	va_end(args);
}

/**
 * \brief Warning Message - Something the user should know
 */
void Log_Warning(char *Ident, char *Message, ...)
{
	va_list	args;
	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_WARNING, Message, args);
	va_end(args);
}

/**
 * \brief Notice Message - Something the user might like to know
 */
void Log_Notice(char *Ident, char *Message, ...)
{
	va_list	args;	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_NOTICE, Message, args);
	va_end(args);
}

/**
 * \brief Log Message - Possibly useful information
 */
void Log_Log(char *Ident, char *Message, ...)
{
	va_list	args;	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_LOG, Message, args);
	va_end(args);
}

/**
 * \brief Debug Message - Only a developer would want this info
 */
void Log_Debug(char *Ident, char *Message, ...)
{
	va_list	args;	
	va_start(args, Message);
	Log_AddEvent(Ident, LOG_LEVEL_DEBUG, Message, args);
	va_end(args);
}

/**
 * \brief Converts a string into a 64-bit ident
 */
static Uint64 Log_Int_GetIdent(const char *Str)
{
	Uint64	ret = 0;
	 int	i;
	char	ch;
	
	for( i = 0; Str[i] && i < 7; i++ )
	{
		ch = Str[i];
		
		if(ch < ' ')
			ch = '?';
		else if(ch > '~')
			ch = '?';
		
		ret |= (Uint64)ch << 8*i;
	}
	
	for( ; i < 7; i++ )
		ret |= 0x20 << (8*i);
	
	return ret;
}
