/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * deferred_calls.c
 * - UDI Deferred call code
 *
 * Used to prevent excessive recursion causing stack exhaustion
 */
#define DEBUG	0
#include <acess.h>
#include <udi.h>
#include "udi_internal.h"
#include <workqueue.h>

// === PROTOTYPES ===
void UDI_int_Deferred_UnmashalCb(tUDI_DeferredCall *Call);
void UDI_int_Deferred_UnmashalCbU8(tUDI_DeferredCall *Call);
void UDI_int_Deferred_UnmashalCbS(tUDI_DeferredCall *Call);

// === GLOBALS ===
tWorkqueue	gUDI_DeferredWorkQueue;

// === CODE ===
void UDI_int_DeferredThread(void *unused)
{
	Threads_SetName("UDI Deferred");
	Workqueue_Init(&gUDI_DeferredWorkQueue, "UDI Deferred", offsetof(tUDI_DeferredCall, Next));
	
	for(;;)
	{
		tUDI_DeferredCall *call = Workqueue_GetWork(&gUDI_DeferredWorkQueue);
		
		if( !call )
		{
			Log_Notice("UDI", "Deferred thread worken with no work");
			continue ;
		}
		
		// Ummarshal calls handler and frees the call
		call->Unmarshal( call );
	}
}

void UDI_int_AddDeferred(tUDI_DeferredCall *Call)
{
	Workqueue_AddWork(&gUDI_DeferredWorkQueue, Call);
}

tUDI_DeferredCall *UDI_int_AllocDeferred(udi_cb_t *cb, udi_op_t *handler, tUDI_DeferredUnmarshal *Unmarshal, size_t extra)
{
	tUDI_DeferredCall *ret = NEW( tUDI_DeferredCall, + extra );
	ret->Unmarshal = Unmarshal;
	ret->Handler = handler;
	ret->cb = cb;
	return ret;
} 


typedef void	udi_op_cb_t(udi_cb_t *gcb);
void UDI_int_MakeDeferredCb(udi_cb_t *cb, udi_op_t *handler)
{
	tUDI_DeferredCall *call = UDI_int_AllocDeferred(cb, handler, UDI_int_Deferred_UnmashalCb, 0);
	UDI_int_AddDeferred(call);
}
void UDI_int_Deferred_UnmashalCb(tUDI_DeferredCall *Call)
{
	UDI_int_ChannelReleaseFromCall(Call->cb);
	((udi_op_cb_t*)Call->Handler)(Call->cb);
	free(Call);
}

typedef void	udi_op_cbu8_t(udi_cb_t *gcb, udi_ubit8_t arg1);
struct sData_CbU8 {
	udi_ubit8_t	arg1;
};
void UDI_int_MakeDeferredCbU8(udi_cb_t *cb, udi_op_t *handler, udi_ubit8_t arg1)
{
	struct sData_CbU8	*data;
	tUDI_DeferredCall *call = UDI_int_AllocDeferred(cb, handler, UDI_int_Deferred_UnmashalCbU8, sizeof(*data));
	data = (void*)(call + 1);
	data->arg1 = arg1;
	UDI_int_AddDeferred(call);
}
void UDI_int_Deferred_UnmashalCbU8(tUDI_DeferredCall *Call)
{
	struct sData_CbU8 *data = (void*)(Call+1);
	UDI_int_ChannelReleaseFromCall(Call->cb);
	((udi_op_cbu8_t*)Call->Handler)(Call->cb, data->arg1);
	free(Call);
}

typedef void	udi_op_cbs_t(udi_cb_t *gcb, udi_status_t status);
struct sData_CbS {
	udi_status_t	status;
};
void UDI_int_MakeDeferredCbS(udi_cb_t *cb, udi_op_t *handler, udi_status_t status)
{
	struct sData_CbS	*data;
	tUDI_DeferredCall *call = UDI_int_AllocDeferred(cb, handler, UDI_int_Deferred_UnmashalCbS, sizeof(*data));
	data = (void*)(call + 1);
	data->status = status;
	UDI_int_AddDeferred(call);
}
void UDI_int_Deferred_UnmashalCbS(tUDI_DeferredCall *Call)
{
	struct sData_CbS *data = (void*)(Call+1);
	UDI_int_ChannelReleaseFromCall(Call->cb);
	((udi_op_cbs_t*)Call->Handler)(Call->cb, data->status);
	free(Call);
}

