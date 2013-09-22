/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/serial.c
 * - Common serial port code
 */
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_serial.h>
#include <drv_pty.h>

// === TYPES ===
struct sSerialPort
{
	tPTY	*PTY;
	tSerial_OutFcn	OutputFcn;
	void	*OutHandle;
};

// === PROTOTYPES ===
 int	Serial_Install(char **Arguments);
//tSerialPort	*Serial_CreatePort( tSerial_OutFcn output, void *handle );
//void	Serial_ByteReceived(tSerialPort *Port, char Ch);
void	Serial_int_PTYOutput(void *Handle, size_t Length, const void *Buffer);
 int	Serial_int_PTYSetArrib(void *Handle, const struct ptymode *Mode);
 int	Serial_int_PTYSetDims(void *Handle, const struct ptydims *Dims);
void	Serial_int_OutputDebug(void *unused, char ch);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, Serial, Serial_Install, NULL, "PTY", NULL);
tSerialPort	*gSerial_KernelDebugPort;

// === CODE ===
int Serial_Install(char **Arguments)
{
	gSerial_KernelDebugPort = Serial_CreatePort( Serial_int_OutputDebug, NULL );
	return 0;
}

tSerialPort *Serial_CreatePort(tSerial_OutFcn output, void *handle)
{
	tSerialPort	*ret = malloc( sizeof(tSerialPort) );
	// TODO: Make PTY code handle 'serial#' and auto-number
	ret->PTY = PTY_Create("serial0", ret, Serial_int_PTYOutput, Serial_int_PTYSetDims, Serial_int_PTYSetArrib);
	ret->OutputFcn = output;
	ret->OutHandle = handle;
	struct ptymode mode = {
		.OutputMode = PTYBUFFMT_TEXT,
		.InputMode = PTYIMODE_CANON|PTYIMODE_ECHO
	};
	struct ptydims dims = {
		.W = 80, .H = 25,
		.PW = 0, .PH = 0
	};
	PTY_SetAttrib(ret->PTY, &dims, &mode, 0);
	return ret;
}

void Serial_ByteReceived(tSerialPort *Port, char Ch)
{
	if( !Port )
		return ;
	if( Ch == '\r' )
		Ch = '\n';
	PTY_SendInput(Port->PTY, &Ch, 1);
}

void Serial_int_PTYOutput(void *Handle, size_t Length, const void *Buffer)
{
	tSerialPort	*Port = Handle;
	const char	*buf = Buffer;
	for( int i = 0; i < Length; i ++ )
		Port->OutputFcn( Port->OutHandle, *buf++ );
}
int Serial_int_PTYSetArrib(void *Handle, const struct ptymode *Mode)
{
	return 0;
}
int Serial_int_PTYSetDims(void *Handle, const struct ptydims *Dims)
{
	return 0;
}

void Serial_int_OutputDebug(void *unused, char ch)
{
	Debug_PutCharDebug(ch);
}

