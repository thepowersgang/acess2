/*
 * Acess2 EHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ehci.h
 * - ECHI Header
 */
#ifndef _EHCI_H_
#define _EHCI_H_

#include <threads.h>

typedef struct sEHCI_Controller	tEHCI_Controller;
typedef struct sEHCI_Endpoint	tEHCI_Endpoint;

#define PERIODIC_SIZE	1024

#include "ehci_hw.h"

#define PID_OUT 	0
#define PID_IN  	1
#define PID_SETUP	2

#define TD_POOL_SIZE	(PAGE_SIZE/sizeof(tEHCI_qTD))
// - 256 addresses * 16 endpoints
#define QH_POOL_SIZE	(256*16)
#define QH_POOL_PAGES	(QH_POOL_SIZE*sizeof(tEHCI_QH)/PAGE_SIZE)
#define QH_POOL_NPERPAGE	(PAGE_SIZE/sizeof(tEHCI_QH))

struct sEHCI_Controller
{
	tUSBHub	*RootHub;
	tThread	*InterruptThread;
	 int	nPorts;

	tPAddr	PhysBase;
	tEHCI_CapRegs	*CapRegs;
	tEHCI_OpRegs	*OpRegs;

	tEHCI_qTD	*DeadTD;
	tEHCI_QH	*DeadQH;

	 int	InterruptLoad[PERIODIC_SIZE];
	
	tMutex	lReclaimList;
	tEHCI_QH	*ReclaimList;

	tMutex	lAsyncSchedule;	

	tMutex  	PeriodicListLock;
	Uint32  	*PeriodicQueue;
	tEHCI_QH	*PeriodicQueueV[PERIODIC_SIZE];
	
	tMutex	QHPoolMutex;
	tEHCI_QH	*QHPools[QH_POOL_PAGES];	// [PAGE_SIZE/64]
	tMutex	TDPoolMutex;
	tEHCI_qTD	*TDPool;	// [TD_POOL_SIZE]
};

struct sEHCI_Endpoint
{
	bool	NextToggle;
	Uint16	EndpointID;
	Sint8	PeriodPow2;
	Uint8	InterruptOfs;
	size_t	MaxPacketSize;
	tEHCI_QH	*ActiveQHs;
};

#endif

