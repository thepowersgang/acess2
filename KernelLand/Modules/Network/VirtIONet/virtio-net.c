/*
 * Acess2 VirtIO Network Driver
 * - By John Hodge (thePowersGang)
 *
 * virtio-net.c
 * - Driver Core
 */
#define DEBUG	0
#define VERSION	VER2(1,0)
#include <acess.h>
#include <modules.h>
#include <semaphore.h>
#include <drv_pci.h>
#include <IPStack/include/adapters_api.h>
#include "virtio-net.h"
#include <Libraries/VirtIO/include/virtio.h>

#define NRXBUFS	4

// === TYPEDEFS ===
typedef struct sVirtIONet_Dev	tVirtIONet_Dev;

// === STRUCTURES ===
struct sVirtIONet_Dev
{
	Uint32	Features;
	tSemaphore	RXPacketSem;
	void	*RXBuffers[NRXBUFS];
};

// === PROTOTYPES ===
 int	VirtIONet_Install(char **Arguments);
 int	VirtIONet_Cleanup(void);
void	VirtIONet_AddCard(Uint16 IOBase, Uint IRQ);
 int	VirtIONet_RXQueueCallback(tVirtIO_Dev *Dev, int ID, size_t UsedBytes, void *Handle);
tIPStackBuffer	*VirtIONet_WaitForPacket(void *Ptr);
 int	VirtIONet_TXQueueCallback(tVirtIO_Dev *Dev, int ID, size_t UsedBytes, void *Handle);
 int	VirtIONet_SendPacket(void *Ptr, tIPStackBuffer *Buffer);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VirtIONet, VirtIONet_Install, VirtIONet_Cleanup, "IPStack", "VirtIOCommon", NULL);
tIPStack_AdapterType	gVirtIONet_AdapterType = {
	.Name = "VirtIONet",
	.Type = ADAPTERTYPE_ETHERNET_1G,	// TODO: Differentiate differnet wire protos and speeds
	.Flags = ADAPTERFLAG_OFFLOAD_MAC,	// TODO: IP/TCP/UDP checksum offloading
	.SendPacket = VirtIONet_SendPacket,
	.WaitForPacket = VirtIONet_WaitForPacket
};

// === CODE ===
int VirtIONet_Install(char **Arguments)
{
	 int	pcidev = -1;
	// Find network devices
	while( (pcidev = PCI_GetDeviceByClass(0x020000, 0xFF0000, pcidev)) != -1 )
	{
		Uint16	ven, dev;
		PCI_GetDeviceInfo(pcidev, &ven, &dev, NULL);
		LOG("Device %i: %x/%x", pcidev, ven, dev);
		// 0x1AF4:(0x1000-0x103F) are VirtIO devices
		if( ven != 0x1AF4 || (dev & 0xFFC0) != 0x1000 )
			continue ;
		Uint16	subsys_id;
		PCI_GetDeviceSubsys(pcidev, &ven, &subsys_id);
		if( subsys_id != 1 ) {
			Log_Notice("VirtIONet", "Device with network PCI class but not subsys (%i!=1)", subsys_id);
			continue ;
		}

		Uint8	irq = PCI_GetIRQ(pcidev);
		// TODO: Detect bad IRQ
		
		Uint32	iobase = PCI_GetBAR(pcidev, 0);
		if( iobase == 0 ) {
			// oops
		}
		
		VirtIONet_AddCard(iobase & ~1, irq);
	}
	return 0;
}

int VirtIONet_Cleanup(void)
{
	Log_Warning("VirtIONet", "TODO: Clean up before module unload");
	return 0;
}

void VirtIONet_AddCard(Uint16 IOBase, Uint IRQ)
{
	ENTER("xMMIOBase xIRQ", IOBase, IRQ);
	// Should be a VirtIO Network device
	tVirtIO_Dev *dev = VirtIO_InitDev(
		IOBase, IRQ,
		VIRTIO_NET_F_MAC|VIRTIO_NET_F_STATUS|VIRTIO_NET_F_CSUM|VIRTIO_NET_F_MRG_RXBUF
			|VIRTIO_F_NOTIFY_ON_EMPTY,
		3,
		sizeof(struct sVirtIONet_Dev)
		);
	if( !dev ) {
		// Oops?
	}
	tVirtIONet_Dev	*ndev = VirtIO_GetDataPtr(dev);
	Semaphore_Init(&ndev->RXPacketSem, 0, NRXBUFS, "VirtIONet", "RXSem");
	ndev->Features = VirtIO_GetFeatures(dev);
	
	Uint8	mac[6];
	if( ndev->Features & VIRTIO_NET_F_MAC ) {
		for( int i = 0; i < 6; i ++ )
			mac[i] = VirtIO_GetDevConfig(dev, 8, i);
		Log_Log("VirtIONet", "Card %x,%i - MAC %02x:%02x:%02x:%02x:%02x:%02x",
			IOBase, IRQ,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	else {
		// x[2,6,A,E] xx xx xx xx xx
		mac[0] = 0x0A;
		mac[1] = 0xCE;
		mac[2] = 0x55;
		mac[3] = rand() & 0xFF;
		mac[4] = rand() & 0xFF;
		mac[5] = rand() & 0xFF;
		Log_Log("VirtIONet", "Card %x,%i - Random MAC %02x:%02x:%02x:%02x:%02x:%02x",
			IOBase, IRQ,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	// NoAutoRel=1 : Keep RX buffers about until worker is done with them
	VirtIO_SetQueueCallback(dev, 0, VirtIONet_RXQueueCallback, 1);
	VirtIO_SetQueueCallback(dev, 1, VirtIONet_TXQueueCallback, 0);

	// Set up RX buffers
	for( int i = 0; i < NRXBUFS; i ++ )
	{
		ndev->RXBuffers[i] = MM_AllocDMA(1, -1, NULL);
		LOG("RxBuf %i/%i: %p", i, NRXBUFS, ndev->RXBuffers[i]);
		VirtIO_ReceiveBuffer(dev, 0, PAGE_SIZE, ndev->RXBuffers[i], NULL);
	}

	// Register with IPStack
	LOG("Register");
	// TODO: Save the returned pointer to do deregister later	
	IPStack_Adapter_Add(&gVirtIONet_AdapterType, dev, mac);
	LEAVE('-');
}

int VirtIONet_RXQueueCallback(tVirtIO_Dev *Dev, int ID, size_t UsedBytes, void *Handle)
{
	tVirtIONet_Dev	*NDev = VirtIO_GetDataPtr(Dev);
	LOG("Signalling");
	Semaphore_Signal(&NDev->RXPacketSem, 1);
	// 1: Don't pop the qdesc
	return 1;
}

void VirtIONet_ReleaseRX(void *Arg, size_t HeadLen, size_t FootLen, const void *Data)
{
	tVirtIO_Buf	*buf = Arg;
	tVirtIO_Dev	*dev = VirtIO_GetBufferDev(buf);
	// Re-add RX buffer
	VirtIO_ReleaseBuffer(buf);
	void	*bufptr = (void*)((tVAddr)Data & ~(PAGE_SIZE-1));
	VirtIO_ReceiveBuffer(dev, 0, PAGE_SIZE, bufptr, NULL);
}

tIPStackBuffer *VirtIONet_WaitForPacket(void *Ptr)
{
	tVirtIO_Dev	*VIODev = Ptr;
	tVirtIONet_Dev	*NDev = VirtIO_GetDataPtr(VIODev);
	
	if( Semaphore_Wait(&NDev->RXPacketSem, 1) != 1 ) {
		return NULL;
	}
	
	size_t	size;
	const void	*buf;
	tVirtIO_Buf	*id = VirtIO_PopBuffer(VIODev, VIRTIONET_QUEUE_RX, &size, &buf);
	if( id == NULL )
	{
		// Pop failed, nothing there (possibly not error condition)
		return NULL;
	}
	
	const tVirtIONet_PktHdr	*hdr = buf;
	 int	nbufs = (NDev->Features & VIRTIO_NET_F_MRG_RXBUF) ? hdr->NumBuffers : 1;
	size_t	dataofs = (NDev->Features & VIRTIO_NET_F_MRG_RXBUF) ? sizeof(*hdr) : sizeof(*hdr)-2;
	
	ASSERTCR(nbufs, >=, 1, NULL);
	ASSERTCR(size, >, dataofs, NULL);

	tIPStackBuffer	*ret = IPStack_Buffer_CreateBuffer(nbufs);
	IPStack_Buffer_AppendSubBuffer(ret, 0, size-dataofs, (const char*)buf + dataofs, VirtIONet_ReleaseRX, id);

	// TODO: This will break if descriptors end up being chained

	for( int i = 1; i < nbufs; i ++ )
	{
		while( NULL == (id = VirtIO_PopBuffer(VIODev, VIRTIONET_QUEUE_RX, &size, &buf)) )
			Semaphore_Wait(&NDev->RXPacketSem, 1);
		IPStack_Buffer_AppendSubBuffer(ret, 0, size, buf, VirtIONet_ReleaseRX, id);
	}
	return ret;
}

int VirtIONet_TXQueueCallback(tVirtIO_Dev *Dev, int ID, size_t UsedBytes, void *Handle)
{
	if( Handle ) {
		LOG("Unlock TX'd buffer %p", Handle);
		IPStack_Buffer_UnlockBuffer(Handle);
	}
	return 0;
}

int VirtIONet_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	tVirtIO_Dev	*VIODev = Ptr;
	tVirtIONet_Dev	*NDev = VirtIO_GetDataPtr(VIODev);
	
	 int	nBufs = 0;
	for( int idx = -1; (idx = IPStack_Buffer_GetBuffer(Buffer, idx, NULL, NULL)) != -1; )
		nBufs ++;
	
	tVirtIONet_PktHdr	hdr;

	hdr.Flags = 0;
	// GSO (TODO: Acess needs to support this)
	hdr.GSOType = 0;
	hdr.HeaderLen = 0;
	hdr.GSOSize = 0;
	// IP/TCP checksumming?
	hdr.CSumStart = 0;
	hdr.CSumOffset = 0;
	
	hdr.NumBuffers = 0;
	
	size_t	buflens[1+nBufs];
	const void	*bufptrs[1+nBufs];
	buflens[0] = sizeof(hdr) - ((NDev->Features & VIRTIO_NET_F_MRG_RXBUF) ? 0 : 2);
	bufptrs[0] = &hdr;
	 int	i = 1;
	for( int idx = -1; (idx = IPStack_Buffer_GetBuffer(Buffer, idx, &buflens[i], &bufptrs[i])) != -1; )
		i ++;
	
	IPStack_Buffer_LockBuffer(Buffer);
	VirtIO_SendBuffers(VIODev, VIRTIONET_QUEUE_TX, nBufs+1, buflens, bufptrs, Buffer);
	
	// Wait until TX completes
	IPStack_Buffer_LockBuffer(Buffer);
	IPStack_Buffer_UnlockBuffer(Buffer);
	
	return 0;
}

