/*
 * Acess2 UDI Layer
 */
#define DEBUG	0
#define VERSION	((0<<8)|1)
#include <acess.h>
#include <modules.h>
#include <udi.h>
#include "udi_internal.h"

// === PROTOTYPES ===
 int	UDI_Install(char **Arguments);
 int	UDI_DetectDriver(void *Base);
 int	UDI_LoadDriver(void *Base);

tUDI_DriverInstance	*UDI_CreateInstance(tUDI_DriverModule *DriverModule);
tUDI_DriverRegion	*UDI_InitRegion(tUDI_DriverInstance *Inst, udi_ubit16_t Index, udi_ubit16_t Type, size_t RDataSize);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, UDI, UDI_Install, NULL, NULL);
tModuleLoader	gUDI_Loader = {
	NULL, "UDI", UDI_DetectDriver, UDI_LoadDriver, NULL
};

// === CODE ===
/**
 * \fn int UDI_Install(char **Arguments)
 * \brief Stub intialisation routine
 */
int UDI_Install(char **Arguments)
{
	Module_RegisterLoader( &gUDI_Loader );
	return MODULE_ERR_OK;
}

/**
 * \brief Detects if a driver should be loaded by the UDI subsystem
 */
int UDI_DetectDriver(void *Base)
{
	Uint	unused;
	
	if( Binary_FindSymbol(Base, "udi_init_info", &unused) == 0) {
		return 0;
	}
	if( Binary_FindSymbol(Base, "_udiprops", &unused) == 0 ) {
		Log_Warning("UDI", "Driver has udi_init_info, but no _udiprops symbol");
		return 0;
	}
	if( Binary_FindSymbol(Base, "_udiprops_end", &unused) == 0) {
		Log_Warning("UDI", "Driver has udi_init_info, but no _udiprops_end symbol");
		return 0;
	}
	
	return 1;
}

/**
 * \fn int UDI_LoadDriver(void *Base)
 */
int UDI_LoadDriver(void *Base)
{
	udi_init_t	*info;
	char	*udiprops = NULL;
	char	*udiprops_end = 0;
	
	ENTER("pBase", Base);
	
	if( Binary_FindSymbol(Base, "udi_init_info", (Uint*)&info) == 0) {
		Binary_Unload(Base);
		LEAVE('i', 0);
		return 0;
	}
	
	Binary_FindSymbol(Base, "_udiprops", (Uint*)&udiprops);
	Binary_FindSymbol(Base, "_udiprops_end", (Uint*)&udiprops_end);

	#if 0	
	Log("primary_init_info = %p = {", info->primary_init_info);
	{
		Log(" .mgmt_ops = %p = {", info->primary_init_info->mgmt_ops);
		Log("  .usage_ind_op: %p() - 0x%02x",
			info->primary_init_info->mgmt_ops->usage_ind_op,
			info->primary_init_info->mgmt_op_flags[0]
			);
		Log("  .enumerate_req_op: %p() - 0x%02x",
			info->primary_init_info->mgmt_ops->enumerate_req_op,
			info->primary_init_info->mgmt_op_flags[1]
			);
		Log("  .devmgmt_req_op: %p() - 0x%02x",
			info->primary_init_info->mgmt_ops->devmgmt_req_op,
			info->primary_init_info->mgmt_op_flags[2]
			);
		Log("  .final_cleanup_req_op: %p() - 0x%02x",
			info->primary_init_info->mgmt_ops->final_cleanup_req_op,
			info->primary_init_info->mgmt_op_flags[3]
			);
		Log(" }");
		Log(" .mgmt_scratch_requirement = 0x%x", info->primary_init_info->mgmt_scratch_requirement);
		Log(" .enumeration_attr_list_length = 0x%x", info->primary_init_info->enumeration_attr_list_length);
		Log(" .rdata_size = 0x%x", info->primary_init_info->rdata_size);
		Log(" .child_data_size = 0x%x", info->primary_init_info->child_data_size);
		Log(" .per_parent_paths = 0x%x", info->primary_init_info->per_parent_paths);
	}
	Log("}");
	Log("secondary_init_list = %p {", info->secondary_init_list);
	for( int i = 0; info->secondary_init_list && info->secondary_init_list[i].region_idx; i ++ )
	{
		Log(" [%i] = { .region_idx=%i, .rdata_size=0x%x }",
			info->secondary_init_list[i].region_idx, info->secondary_init_list[i].rdata_size);
	}
	LOG("}");
	Log("ops_init_list = %p", info->ops_init_list);
	
	for( int i = 0; info->ops_init_list[i].ops_idx; i++ )
	{
		Log("info->ops_init_list[%i] = {", i);
		Log(" .ops_idx = 0x%x", info->ops_init_list[i].ops_idx);
		Log(" .meta_idx = 0x%x", info->ops_init_list[i].meta_idx);
		Log(" .meta_ops_num = 0x%x", info->ops_init_list[i].meta_ops_num);
		Log(" .chan_context_size = 0x%x", info->ops_init_list[i].chan_context_size);
		Log(" .ops_vector = %p", info->ops_init_list[i].ops_vector);
//		Log(" .op_flags = %p", info->ops_init_list[i].op_flags);
		Log("}");
	}
	#endif

	
	// TODO: Multiple modules?
	tUDI_DriverModule *driver_module = NEW(tUDI_DriverModule,);
	driver_module->InitInfo = info;

	// - Parse udiprops
	{
		char	**udipropsptrs;
		
		Log_Debug("UDI", "udiprops = %p, udiprops_end = %p", udiprops, udiprops_end);
		size_t	udiprops_size = udiprops_end - udiprops;
		
		int nLines = 1;
		for( int i = 0; i < udiprops_size; i++ )
		{
			if( udiprops[i] == '\0' )
				nLines ++;
		}
		
		Log_Debug("UDI", "nLines = %i", nLines);
		
		udipropsptrs = NEW(char*,*nLines);
		int line = 0;
		udipropsptrs[line++] = udiprops;
		for( int i = 0; i < udiprops_size; i++ )
		{
			if( udiprops[i] == '\0' ) {
				udipropsptrs[line++] = &udiprops[i+1];
			}
		}
		if(udipropsptrs[line-1] == &udiprops[udiprops_size])
			nLines --;
		
		for( int i = 0; i < nLines; i ++ )
		{
		}
		
		// Parse out:
		// 'message' into driver_module->Messages
		// 'region' into driver_module->RegionTypes
		// 'module' into driver_module->ModuleName
		
		 int	nMessages = 0;
		 int	nLocales = 1;
		 int	nRegionTypes = 0;
		for( int i = 0; i < nLines; i ++ )
		{
			const char *str = udipropsptrs[i];
			if( strncmp("module ", str, 7) == 0 ) {
				driver_module->ModuleName = str + 7;
			}
			else if( strncmp("message ", str, 8) == 0 ) {
				nMessages ++;
			}
			else if( strncmp("locale ", str, 7) == 0 ) {
				nLocales ++;
			}
			else if( strncmp("region ", str, 7) == 0 ) {
				nRegionTypes ++;
			}
		}

		// Allocate structures
		driver_module->nMessages = nMessages;
		driver_module->Messages = NEW(tUDI_PropMessage, *nMessages);
		driver_module->nRegionTypes = nRegionTypes;
		driver_module->RegionTypes = NEW(tUDI_PropRegion, *nRegionTypes);

		// Populate
		 int	cur_locale = 0;
		 int	msg_index = 0;
		for( int i = 0; i < nLines; i ++ )
		{
			const char *str = udipropsptrs[i];
			if( strncmp("module ", str, 7) == 0 ) {
				driver_module->ModuleName = str + 7;
			}
			else if( strncmp("message ", str, 8) == 0 ) {
				tUDI_PropMessage *msg = &driver_module->Messages[msg_index++];
				char	*end;
				msg->locale = cur_locale;
				msg->index = strtoul(str+8, &end, 10);
				if( *end != ' ' && *end != '\t' ) {
					// Oops.
					continue ;
				}
				while( isspace(*end) )
					end ++;
				msg->string = end;
				
				Log_Debug("UDI", "Message %i/%i: '%s'", msg->locale, msg->index, msg->string);
			}
			else if( strncmp("locale ", str, 7) == 0 ) {
				// TODO: Set locale
				cur_locale = 1;
			}
			else if( strncmp("region ", str, 7) == 0 ) {
				nRegionTypes ++;
			}
			else {
				Log_Debug("UDI", "udipropsptrs[%i] = '%s'", i, udipropsptrs[i]);
			}
		}
		
		// Sort message list
		// TODO: Sort message list
	}

	 int	nSecondaryRgns = 0;
	for( int i = 0; info->secondary_init_list && info->secondary_init_list[i].region_idx; i ++ )
		nSecondaryRgns ++;
	driver_module->nSecondaryRegions = nSecondaryRgns;
	
	// Create initial driver instance
	tUDI_DriverInstance *inst = UDI_CreateInstance(driver_module);

	for( int i = 0; i < 1+driver_module->nSecondaryRegions; i ++ )
		Log("Rgn %i: %p", i, inst->Regions[i]);

	// Send usage indication
	udi_usage_cb_t ucb;
	memset(&ucb, 0, sizeof(ucb));
	ucb.gcb.channel = inst->ManagementChannel;
	udi_usage_ind(&ucb, UDI_RESOURCES_NORMAL);
	
	return 0;
}

tUDI_DriverInstance *UDI_CreateInstance(tUDI_DriverModule *DriverModule)
{
	tUDI_DriverInstance	*inst = NEW_wA(tUDI_DriverInstance, Regions, (1+DriverModule->nSecondaryRegions));
	inst->Module = DriverModule;
	inst->Regions[0] = UDI_InitRegion(inst, 0, 0, DriverModule->InitInfo->primary_init_info->rdata_size);
	udi_secondary_init_t	*sec_init = DriverModule->InitInfo->secondary_init_list;
	if( sec_init )
	{
		for( int i = 0; sec_init[i].region_idx; i ++ )
		{
			inst->Regions[1+i] = UDI_InitRegion(inst, i, sec_init[i].region_idx, sec_init[i].rdata_size);
		}
	}

	inst->ManagementChannel = UDI_CreateChannel(METALANG_MGMT, 0, NULL, 0, inst, 0);
	
	return inst;
}

tUDI_DriverRegion *UDI_InitRegion(tUDI_DriverInstance *Inst, udi_ubit16_t Index, udi_ubit16_t Type, size_t RDataSize)
{
//	ASSERTCR( RDataSize, <=, UDI_MIN_ALLOC_LIMIT, NULL );
	ASSERTCR( RDataSize, >, sizeof(udi_init_context_t), NULL );
	tUDI_DriverRegion	*rgn = NEW(tUDI_DriverRegion,+RDataSize);
	rgn->InitContext = (void*)(rgn+1);
	rgn->InitContext->region_idx = Type;
//	rgn->InitContext->limits
	return rgn;
}

