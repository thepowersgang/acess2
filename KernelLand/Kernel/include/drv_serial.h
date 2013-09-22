/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/serial.c
 * - Common serial port code
 */
#ifndef _DRV_SERIAL_H_
#define _DRV_SERIAL_H_

typedef void	(*tSerial_OutFcn)(void *Handle, char Ch);
typedef struct sSerialPort	tSerialPort;

extern tSerialPort	*gSerial_KernelDebugPort;

extern tSerialPort	*Serial_CreatePort( tSerial_OutFcn output, void *handle );
extern void	Serial_ByteReceived(tSerialPort *Port, char Ch);

#endif

