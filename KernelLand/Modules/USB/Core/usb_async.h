/*
 * Acess2 USB Stack
 * - By John Hodge (thePowersGang)
 *
 * usb_async.h
 * - USB ASync operations
 */
#ifndef _USB__USB_ASYNC_H_
#define _USB__USB_ASYNC_H_
#include <workqueue.h>

typedef struct sAsyncOp	tAsyncOp;

struct sAsyncOp
{
	tAsyncOp	*Next;
	tUSBEndpoint	*Endpt;
	 int	Length;
	void	*Data;
};

extern void	USB_AsyncCallback(void *Ptr, void *Buf, size_t Length);

extern tWorkqueue	gUSB_AsyncQueue;;

#endif

