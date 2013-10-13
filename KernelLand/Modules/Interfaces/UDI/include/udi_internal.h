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
#include <stdarg.h>

#define NEW(type,extra) (type*)calloc(sizeof(type)extra,1)
#define NEW_wA(type,fld,cnt)	NEW(type,+(sizeof(((type*)NULL)->fld[0])*cnt))

typedef struct sUDI_PropMessage 	tUDI_PropMessage;
typedef struct sUDI_PropRegion 	tUDI_PropRegion;
typedef struct sUDI_PropDevSpec	tUDI_PropDevSpec;

typedef const struct sUDI_MetaLang	tUDI_MetaLang;

typedef struct sUDI_MetaLangRef 	tUDI_MetaLangRef;
typedef struct sUDI_BindOps	tUDI_BindOps;

typedef struct sUDI_DriverModule	tUDI_DriverModule;
typedef struct sUDI_DriverInstance	tUDI_DriverInstance;
typedef struct sUDI_DriverRegion	tUDI_DriverRegion;

typedef struct sUDI_ChildBinding	tUDI_ChildBinding;

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
	const void	*MeiInfo;
	 int	nCbTypes;
	struct {
		udi_size_t	Size;
		udi_size_t	ChainOfs;
		udi_layout_t	*Layout;
	} CbTypes[];
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
	 int	MessageNum;
	udi_ubit8_t	MetaIdx;
	tUDI_MetaLang	*Metalang;
	 int	nAttribs;
	udi_instance_attr_list_t	Attribs[];
};

struct sUDI_DriverModule
{
	tUDI_DriverModule	*Next;
	void	*Base;

	const udi_init_t	*InitInfo;

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
	
	 int	nChildBindOps;
	tUDI_BindOps	*ChildBindOps;

	 int	nDevices;
	tUDI_PropDevSpec	**Devices;

	 int	nRegions;
};

struct sUDI_DriverInstance
{
	struct sUDI_DriverInstance	*Next;
	tUDI_DriverModule	*Module;
	udi_channel_t	ManagementChannel;
	tUDI_DriverInstance	*Parent;
	tUDI_ChildBinding	*ParentChildBinding;

	 int	CurState;	// Current MA state
	
	tUDI_ChildBinding	*FirstChild;
	tUDI_DriverRegion	*Regions[];
};

struct sUDI_DriverRegion
{
	udi_init_context_t	*InitContext;
};

struct sUDI_ChildBinding
{
	tUDI_ChildBinding	*Next;
	
	udi_ubit32_t	ChildID;
	tUDI_MetaLang	*Metalang;
	tUDI_BindOps	*BindOps;
	
	udi_ops_init_t	*Ops;
	tUDI_DriverInstance	*BoundInstance;
	
	 int	nAttribs;
	udi_instance_attr_list_t	Attribs[];
};

// --- Metalanguages ---
extern tUDI_MetaLang	cMetaLang_Management;

// --- Index to pointer translation ---
extern udi_ops_init_t	*UDI_int_GetOps(tUDI_DriverInstance *Inst, udi_index_t index);
extern tUDI_MetaLang *UDI_int_GetMetaLang(tUDI_DriverModule *Inst, udi_index_t meta_idx);

// --- Channels ---
extern udi_channel_t	UDI_CreateChannel_Blank(tUDI_MetaLang *metalang);
extern udi_channel_t	UDI_CreateChannel_Linked(udi_channel_t orig, udi_ubit8_t spawn_idx);
extern int	UDI_BindChannel_Raw(udi_channel_t channel, bool other_side, tUDI_DriverInstance *inst, udi_index_t region_idx, udi_index_t meta_ops_num,  void *context, const void *ops);
extern int	UDI_BindChannel(udi_channel_t channel, bool other_side, tUDI_DriverInstance *inst, udi_index_t ops, udi_index_t region, void *context, bool is_child_bind, udi_ubit32_t child_ID);
extern tUDI_DriverInstance	*UDI_int_ChannelGetInstance(udi_cb_t *gcb, bool other_side, udi_index_t *region_idx);
extern void	UDI_int_ChannelSetContext(udi_channel_t channel, void *context);
extern const void	*UDI_int_ChannelPrepForCall(udi_cb_t *gcb, tUDI_MetaLang *metalang, udi_index_t meta_ops_num);
extern void	UDI_int_ChannelFlip(udi_cb_t *gcb);
extern void	UDI_int_ChannelReleaseFromCall(udi_cb_t *gcb);

// --- Async Calls ---
typedef struct sUDI_DeferredCall	tUDI_DeferredCall;
typedef void	tUDI_DeferredUnmarshal(tUDI_DeferredCall *Call);
struct sUDI_DeferredCall
{
	struct sUDI_DeferredCall	*Next;
	tUDI_DeferredUnmarshal	*Unmarshal;
	udi_cb_t	*cb;
	udi_op_t	*Handler;
	// ...
};
extern void	UDI_int_DeferredThread(void *unused);	// Worker started by main.c
extern void	UDI_int_AddDeferred(tUDI_DeferredCall *Call);
extern void	UDI_int_MakeDeferredCb(udi_cb_t *cb, udi_op_t *handler);

extern void	UDI_int_MakeDeferredCbU8(udi_cb_t *cb, udi_op_t *handler, udi_ubit8_t arg1);
extern void	UDI_int_MakeDeferredCbS(udi_cb_t *cb, udi_op_t *handler, udi_status_t status);

// --- CBs ---
extern void *udi_cb_alloc_internal(tUDI_DriverInstance *Inst, udi_ubit8_t bind_cb_idx, udi_channel_t channel);
extern udi_cb_t	*udi_cb_alloc_internal_v(tUDI_MetaLang *Meta, udi_index_t MetaCBNum, size_t inline_size, size_t scratch_size, udi_channel_t channel);
extern tUDI_MetaLang	*UDI_int_GetCbType(udi_cb_t *gcb, udi_index_t *meta_cb_num);

// --- Attribute Management ---
extern udi_instance_attr_type_t udi_instance_attr_get_internal(udi_cb_t *gcb, const char *attr_name, udi_ubit32_t child_ID, void *attr_value, udi_size_t attr_length, udi_size_t *actual_length);

// --- Layout ---
extern size_t	_udi_marshal_step(void *buf, size_t cur_ofs, udi_layout_t **layoutp, va_list *values);
extern size_t	_udi_marshal_values(void *buf, udi_layout_t *layout, va_list values);

// --- Buffers ---
extern udi_buf_t	*_udi_buf_allocate(const void *data, udi_size_t length, udi_buf_path_t path_handle);

#endif

