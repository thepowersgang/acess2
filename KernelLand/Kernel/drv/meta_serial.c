/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/meta_serial.c
 * - Wrapper for 'Serial' ports (UARTs and the like)
 */
#include <fs_devfs.h>
#include <meta_serial.h>

#define DEFAULT_BUFSIZ	64

// === STRUCTS ===
struct sSerialPort
{
	tSerialPort	*Next;
	tSerialPort	*Prev;
	const tSerialHandlers	*Handlers;
	void	*Handle;
	size_t	BufferSize;
	size_t	BufferLen;
	const char *Name;
	void	*Buffer;
};

// === PROTOTYPES ===
int	Serial_Install(char **Arguments);
tSerialPort	*Serial_AddPort(tSerialHandlers *Handlers, void *Handle, const char *Ident);
void	Serial_RemovePort(tSerialPort *Port);
void	Serial_RecieveData(tSerialPort *Port, size_t Bytes, const void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, 1, MetaSerial, Serial_Install, NULL, NULL);
tRWLock	glSerial_ListLock;
tSerialPort	*gSerial_FirstPort;
tSerialPort	*gSerial_LastPort;

// === CODE ===
int Serial_Install(char **Arguments)
{
	return 0;
}

tSerialPort *Serial_AddPort(tSerialHandlers *Handlers, void *Handle, const char *Name)
{
	tSerialPort	*ret = NULL;
	
	// Remove duplicates?
	// Create new structure
	ret = malloc( sizeof(tSerialPort) + strlen(Name) + 1 );

	ret->Handlers = Handlers;
	ret->Handle = Handle;
	ret->Name = (void*)(ret + 1);
	strcpy(ret->Name, Name);
	
	ret->BufferLen = 0;
	ret->Buffer = NULL;
	Serial_SetInputBufferSize(ret, DEFAULT_BUFSIZ);

	// Add to list
	if( RWLock_AcquireWrite(&glSerial_ListLock) ) {
		goto _err;
	}
	ret->Next = NULL;
	ret->Prev = gSerial_LastPort;
	if(gSerial_LastPort)
		gSerial_LastPort->Next = ret;
	else
		gSerial_FirstPort = ret;
	gSerial_LastPort = ret;
	RWLock_Release(&glSerial_ListLock);
	
	return ret;
_err:
	free(ret);
	return NULL;
}

void Serial_RemovePort(tSerialPort *Port)
{
	if( RWLock_AcquireWrite(&glSerial_ListLock) ) {
		// dafuq?
	}

	*(Port->Prev ? &Port->Prev->Next : &gSerial_FirstPort) = Port->Next;
	*(Port->Next ? &Port->Next->Prev : &gSerial_LastPort) = Port->Prev;
	RWLock_Release(&glSerial_ListLock);
	
	free(Port);
}

void Serial_RecieveData(tSerialPort *Port, size_t Bytes, const void *Data)
{
	
}

