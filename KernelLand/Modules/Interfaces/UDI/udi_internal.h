/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_internal.h
 * - Definitions for opaque structures
 */
#ifndef _UDI_INTERNAL_H_
#define _UDI_INTERNAL_H_

#include <stdbool.h>

#define NEW(type,extra) (type*)calloc(sizeof(type)extra,1)
#define NEW_wA(type,fld,cnt)	NEW(type,+(sizeof(((type*)NULL)->fld[0])*cnt))

typedef struct sUDI_PropMessage 	tUDI_PropMessage;
typedef struct sUDI_PropRegion 	tUDI_PropRegion;

typedef const struct sUDI_MetaLang	tUDI_MetaLang;

typedef struct sUDI_MetaLangRef 	tUDI_MetaLangRef;
typedef struct sUDI_BindOps	tUDI_BindOps;

typedef struct sUDI_DriverModule	tUDI_DriverModule;
typedef struct sUDI_DriverInstance	tUDI_DriverInstance;
typedef struct sUDI_DriverRegion	tUDI_DriverRegion;

struct sUDI_PropMessage
{
	 int	locale;
	udi_ubit16_t	index;
	const char	*string;
};

struct sUDI_PropRegion
{
	udi_index_t	RegionIdx;
	enum {
		UDI_REGIONTYPE_NORMAL,
		UDI_REGIONTYPE_FP,
	}	Type;
	enum {
		UDI_REGIONBINDING_STATIC,
		UDI_REGIONBINDING_DYNAMIC,
	}	Binding;
	enum {
		UDI_REGIONPRIO_MED,
		UDI_REGIONPRIO_LO,
		UDI_REGIONPRIO_HI,
	}	Priority;
	enum {
		UDI_REGIONLATENCY_NONOVERRRUNABLE,
		UDI_REGIONLATENCY_POWERFAIL,
		UDI_REGIONLATENCY_OVERRUNNABLE,
		UDI_REGIONLATENCY_RETRYABLE,
		// non_overrunable
		UDI_REGIONLATENCY_NONCTRITICAL,
	}	Latency;
	udi_ubit32_t	OverrunTime;

	udi_index_t	BindMeta;
	udi_index_t	PriBindOps;
	udi_index_t	SecBindOps;
	udi_index_t	BindCb;
};

struct sUDI_MetaLang
{
	const char *Name;
	 int	nOpGroups;
	struct {
		void	*OpList;
	} OpGroups;
};

struct sUDI_MetaLangRef
{
	udi_ubit8_t	meta_idx;
	const char	*interface_name;
	tUDI_MetaLang	*metalang;
	// TODO: pointer to metalanguage structure
};

struct sUDI_BindOps
{
	udi_ubit8_t	meta_idx;
	udi_ubit8_t	region_idx;
	udi_ubit8_t	ops_idx;
	udi_ubit8_t	bind_cb_idx;
};

struct sUDI_PropDevSpec
{
	
};

struct sUDI_DriverModule
{
	tUDI_DriverModule	*Next;
	void	*Base;

	udi_init_t	*InitInfo;

	// Counts of arrays in InitInfo
	 int	nCBInit;
	 int	nOpsInit;
		
	const char	*ModuleName;
	 int	nMessages;
	tUDI_PropMessage	*Messages;	// Sorted list
	
	 int	nRegionTypes;
	tUDI_PropRegion	*RegionTypes;

	 int	nMetaLangs;
	tUDI_MetaLangRef	*MetaLangs;

	 int	nParents;
	tUDI_BindOps	*Parents;
	 int	nRegions;
};

struct sUDI_DriverInstance
{
	tUDI_DriverModule	*Module;
	udi_channel_t	ManagementChannel;
	tUDI_DriverRegion	*Regions[];
};

struct sUDI_DriverRegion
{
	udi_init_context_t	*InitContext;
};


extern tUDI_MetaLang	cMetaLang_Management;


// --- Index to pointer translation ---
extern udi_ops_init_t	*UDI_int_GetOps(tUDI_DriverInstance *Inst, udi_index_t index);
extern tUDI_MetaLang *UDI_int_GetMetaLang(tUDI_DriverInstance *Inst, udi_index_t meta_idx);

// --- Channels ---
extern udi_channel_t	UDI_CreateChannel_Blank(tUDI_MetaLang *metalang);
extern int	UDI_BindChannel_Raw(udi_channel_t channel, bool other_side, udi_index_t meta_ops_num, void *context, const void *ops);
extern int	UDI_BindChannel(udi_channel_t channel, bool other_side, tUDI_DriverInstance *inst, udi_index_t ops, udi_index_t region);
extern const void	*UDI_int_ChannelPrepForCall(udi_cb_t *gcb, tUDI_MetaLang *metalang, udi_index_t meta_ops_num);
extern void	UDI_int_ChannelReleaseFromCall(udi_cb_t *gcb);

// --- Async Calls ---
typedef struct sUDI_DeferredCall	tUDI_DeferredCall;
typedef void	tUDI_DeferredUnmarshal(tUDI_DeferredCall *Call);
struct sUDI_DeferredCall
{
	struct sUDI_DeferredCall	*Next;
	tUDI_DeferredUnmarshal	*Unmarshal;
	udi_op_t	*Handler;
	// ...
};
extern void	UDI_int_AddDeferred(tUDI_DeferredCall *Call);
extern void	UDI_int_MakeDeferredCb(udi_cb_t *cb, udi_op_t *handler);
extern void	UDI_int_MakeDeferredCbU8(udi_cb_t *cb, udi_op_t *handler, udi_ubit8_t arg1);
extern void	UDI_int_MakeDeferredCbS(udi_cb_t *cb, udi_op_t *handler, udi_status_t status);

// --- CBs ---
extern void *udi_cb_alloc_internal(tUDI_DriverInstance *Inst, udi_ubit8_t bind_cb_idx, udi_channel_t channel);


#endif

