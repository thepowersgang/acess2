/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * meta_serial.h
 * - Wrapper for 'Serial' ports (UARTs and the like)
 */
#ifndef _META_SERIAL_H_
#define _META_SERIAL_H_

typedef struct sSerialPort	tSerialPort;
typedef struct sSerialHandler	tSerialHandlers;

typedef	int	(*tSerial_SetFmt)(void *Handle, Uint32 Format);
typedef int	(*tSerial_CanSend)(void *Handle);
typedef size_t	(*tSerial_Send)(void *Handle, size_t Bytes, const void *Data);

struct sSerialHandler
{
	const char	*Class;
	tSerial_SetFmt	SetFmt;
	tSerial_CanSend	CanSend;
	tSerial_Send	Send;
};

extern tSerialPort	*Serial_AddPort(tSerialHandlers *Handlers, void *Handle, const char *Ident);
extern void	Serial_RemovePort(tSerialPort *Port);
extern void	Serial_RecieveData(tSerialPort *Port, size_t Bytes, const void *Data);

#endif
