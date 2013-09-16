/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_internal.h
 * - Definitions for opaque structures
 */
#ifndef _UDI_INTERNAL_H_
#define _UDI_INTERNAL_H_

#define NEW(type,extra) (type*)calloc(sizeof(type)extra,1)
#define NEW_wA(type,fld,cnt)	NEW(type,+(sizeof(((type*)NULL)->fld[0])*cnt))

typedef struct sUDI_PropMessage 	tUDI_PropMessage;
typedef struct sUDI_PropRegion 	tUDI_PropRegion;

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


};

struct sUDI_DriverModule
{
	tUDI_DriverModule	*Next;
	void	*Base;

	udi_init_t	*InitInfo;
		
	const char	*ModuleName;
	 int	nMessages;
	tUDI_PropMessage	*Messages;	// Sorted list
	
	 int	nRegionTypes;
	tUDI_PropRegion	*RegionTypes;
	
	 int	nSecondaryRegions;
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

enum eUDI_MetaLang
{
	METALANG_MGMT,
	METALANG_BUS
};

extern udi_channel_t	UDI_CreateChannel(enum eUDI_MetaLang metalang, udi_index_t meta_ops_num,
	tUDI_DriverInstance *ThisEnd, udi_index_t ThisOpsIndex,
	tUDI_DriverInstance *OtherEnd, udi_index_t OtherOpsIndex);

extern const void	*UDI_int_ChannelPrepForCall(udi_cb_t *gcb, enum eUDI_MetaLang metalang);

#endif

