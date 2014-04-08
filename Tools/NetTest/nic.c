/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * nic.c
 * - Acess -> TAP Wrapper
 */
#include <acess.h>
#include <IPStack/include/adapters_api.h>
#include <nettest.h>

// === CONSTANTS ===
static const int MTU = 1520;

// === PROTOTYPES ===
void	NativeNic_int_FreePacket(void *Ptr, size_t pkt_length, size_t Unused, const void *DataPtr);
tIPStackBuffer	*NativeNic_WaitForPacket(void *Ptr);
 int	NativeNic_SendPacket(void *Ptr, tIPStackBuffer *Buffer);

// === GLOBALS ===
tIPStack_AdapterType	gNativeNIC_AdapterType = {
	.Name = "NativeNIC",
	.Type = 0,
	.Flags = 0,
	.SendPacket = NativeNic_SendPacket,
	.WaitForPacket = NativeNic_WaitForPacket
	};

// === CODE ===
int NativeNic_AddDev(char *DevDesc)
{
	Uint8	macaddr[6];
	// Parse descriptor into mac address and path
	// 123456789ABC:tun:ifname
	// 123456789ABC:unix:/path
	// 123456789ABC:file:/path
	// 123456789ABC:tcp:host_desc
	char *colonpos = strchr(DevDesc, ':');
	if( !colonpos )
		return -1;
	if( UnHex(macaddr, 6, DevDesc) != 6 )
		return -1;
	
	char *class = colonpos + 1;
	colonpos = strchr(class, ':');
	void *ptr;
	if( strncmp(class, "tun:", 4) == 0 ) {
		ptr = NetTest_OpenTap(colonpos+1);
	}
	else if( strncmp(class, "unix:", 5) == 0 ) {
		ptr = NetTest_OpenUnix(colonpos+1);
	}
	else {
		Log_Warning("NIC", "Unknown opener '%.*s'", colonpos-class, class);
		ptr = NULL;
	}
	if( !ptr )
		return 1;
	IPStack_Adapter_Add(&gNativeNIC_AdapterType, ptr, macaddr);
	
	return 0;
}

void NativeNic_int_FreePacket(void *Ptr, size_t pkt_length, size_t Unused, const void *DataPtr)
{
	free( (void*)DataPtr );
}

tIPStackBuffer *NativeNic_WaitForPacket(void *Ptr)
{
	char	*buf = malloc( MTU );
	size_t	len;

	len = NetTest_ReadPacket(Ptr, MTU, buf);

	tIPStackBuffer	*ret = IPStack_Buffer_CreateBuffer(1);
	IPStack_Buffer_AppendSubBuffer(ret, len, 0, buf, NativeNic_int_FreePacket, Ptr);
	
	Debug_HexDump("NativeNic: RX", buf, len);
	
	return ret;
}

int NativeNic_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	size_t	len = IPStack_Buffer_GetLength(Buffer);
	
	// Check against MTU
	if( len > MTU )
		return -1;
	
	// Serialise into stack
	char	buf[len];
	IPStack_Buffer_GetData(Buffer, buf, len);

	Debug_HexDump("NativeNic: TX", buf, len);
	
	NetTest_WritePacket(Ptr, len, buf);
	return 0;
}
