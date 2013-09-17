/*
 * Acess2 UDI Layer
 */
#define DEBUG	0
#define VERSION	((0<<8)|1)
#include <acess.h>
#include <modules.h>
#include <udi.h>
#include "udi_internal.h"

// === IMPORTS ===
extern udi_init_t	pci_init;
extern char	pci_udiprops[];
extern size_t	pci_udiprops_size;

// === PROTOTYPES ===
 int	UDI_Install(char **Arguments);
 int	UDI_DetectDriver(void *Base);
 int	UDI_LoadDriver(void *Base);

tUDI_DriverModule	*UDI_int_LoadDriver(void *LoadBase, udi_init_t *info, const char *udiprops, size_t udiprops_size);

tUDI_DriverInstance	*UDI_CreateInstance(tUDI_DriverModule *DriverModule);
tUDI_DriverRegion	*UDI_InitRegion(tUDI_DriverInstance *Inst, udi_ubit16_t Index, udi_ubit16_t Type, size_t RDataSize);
void	UDI_int_BeginEnumeration(tUDI_DriverInstance *Inst);

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

	UDI_int_LoadDriver(NULL, &pci_init, pci_udiprops, pci_udiprops_size);

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
	
	if( Binary_FindSymbol(Base, "udi_init_info", (Uint*)&info) == 0) {
		Binary_Unload(Base);
		return 0;
	}
	
	Binary_FindSymbol(Base, "_udiprops", (Uint*)&udiprops);
	Binary_FindSymbol(Base, "_udiprops_end", (Uint*)&udiprops_end);
	Log_Debug("UDI", "udiprops = %p, udiprops_end = %p", udiprops, udiprops_end);

	UDI_int_LoadDriver(Base, info, udiprops, udiprops_end - udiprops);

	#if 0
	// Create initial driver instance
	tUDI_DriverInstance *inst = UDI_CreateInstance(driver_module);
	
	// Bind to parent
	// TODO: This will move to udi_enumerate_ack handling
	for( int i = 0; i < driver_module->nParents; i ++ )
	{
		tUDI_BindOps	*parent = &driver_module->Parents[i];
		udi_channel_t channel = UDI_CreateChannel_Blank( UDI_int_GetMetaLang(inst, parent->meta_idx) );
		
		UDI_BindChannel(channel,true,  inst, parent->ops_idx, parent->region_idx);
		//UDI_BindChannel(channel,false, parent_inst, parent_chld->ops_idx, parent_chld->region_idx);
		
		udi_cb_t *bind_cb = udi_cb_alloc_internal(inst, parent->bind_cb_idx, channel);
		if( !bind_cb ) {
			Log_Warning("UDI", "Bind CB index is invalid");
			continue ;
		}

		udi_channel_event_cb_t	ev_cb;
		 int	n_handles = driver_module->InitInfo->primary_init_info->per_parent_paths;
		udi_buf_path_t	handles[n_handles];
		memset(&ev_cb, 0, sizeof(ev_cb));
		ev_cb.gcb.channel = channel;
		ev_cb.event = UDI_CHANNEL_BOUND;
		ev_cb.params.parent_bound.bind_cb = bind_cb;
 		ev_cb.params.parent_bound.parent_ID = i+1;
		ev_cb.params.parent_bound.path_handles = handles;
		
		for( int i = 0; i < n_handles; i ++ ) {
			//handles[i] = udi_buf_path_alloc_internal(inst);
			handles[i] = 0;
		}
		
		udi_channel_event_ind(&ev_cb);
	}

	// Send enumeraton request
	#endif
	
	return 0;
}

tUDI_DriverModule *UDI_int_LoadDriver(void *LoadBase, udi_init_t *info, const char *udiprops, size_t udiprops_size)
{
	//UDI_int_DumpInitInfo(info);
	
	// TODO: Multiple modules?
	tUDI_DriverModule *driver_module = NEW(tUDI_DriverModule,);
	driver_module->InitInfo = info;

	// - Parse udiprops
	{
		const char	**udipropsptrs;
		
		
		int nLines = 1;
		for( int i = 0; i < udiprops_size; i++ )
		{
			if( udiprops[i] == '\0' )
				nLines ++;
		}
		
		Log_Debug("UDI", "nLines = %i", nLines);
		
		udipropsptrs = NEW(const char*,*nLines);
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
		
		 int	nLocales = 1;
		 int	nRegionTypes = 0;
		for( int i = 0; i < nLines; i ++ )
		{
			const char *str = udipropsptrs[i];
			if( strncmp("module ", str, 7) == 0 ) {
				driver_module->ModuleName = str + 7;
			}
			else if( strncmp("meta ", str, 5) == 0 ) {
				driver_module->nMetaLangs ++;
			}
			else if( strncmp("message ", str, 8) == 0 ) {
				driver_module->nMessages ++;
			}
			else if( strncmp("locale ", str, 7) == 0 ) {
				nLocales ++;
			}
			else if( strncmp("region ", str, 7) == 0 ) {
				nRegionTypes ++;
			}
			else if( strncmp("parent_bind_ops ", str, 16) == 0 ) {
				driver_module->nParents ++;
			}
		}

		// Allocate structures
		driver_module->Messages    = NEW(tUDI_PropMessage, * driver_module->nMessages);
		driver_module->nRegionTypes = nRegionTypes;
		driver_module->RegionTypes = NEW(tUDI_PropRegion,  * driver_module->nRegionTypes);
		driver_module->MetaLangs   = NEW(tUDI_MetaLangRef, * driver_module->nMetaLangs);
		driver_module->Parents     = NEW(tUDI_BindOps,     * driver_module->nParents);

		// Populate
		 int	cur_locale = 0;
		 int	msg_index = 0;
		 int	ml_index = 0;
		 int	parent_index = 0;
		for( int i = 0; i < nLines; i ++ )
		{
			const char *str = udipropsptrs[i];
			if( strncmp("module ", str, 7) == 0 ) {
				driver_module->ModuleName = str + 7;
			}
			else if( strncmp("meta ", str, 5) == 0 ) {
				tUDI_MetaLangRef *ml = &driver_module->MetaLangs[ml_index++];
				char	*end;
				ml->meta_idx = strtoul(str+5, &end, 10);
				if( *end != ' ' && !end != '\t' ) {
					// TODO: handle 'meta' error
					continue ;
				}
				while( isspace(*end) )
					end ++;
				ml->interface_name = end;
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
				
				//Log_Debug("UDI", "Message %i/%i: '%s'", msg->locale, msg->index, msg->string);
			}
			else if( strncmp("locale ", str, 7) == 0 ) {
				// TODO: Set locale
				cur_locale = 1;
			}
			//else if( strncmp("region ", str, 7) == 0 ) {
			//	nRegionTypes ++;
			//}
			else if( strncmp("parent_bind_ops ", str, 16) == 0 ) {
				tUDI_BindOps	*bind = &driver_module->Parents[parent_index++];
				char *end;
				bind->meta_idx = strtoul(str+16, &end, 10);
				while(isspace(*end))	end++;
				bind->region_idx = strtoul(end, &end, 10);
				while(isspace(*end))	end++;
				bind->ops_idx = strtoul(end, &end, 10);
				while(isspace(*end))	end++;
				bind->bind_cb_idx = strtoul(end, &end, 10);
				Log_Debug("UDI", "Parent bind - meta:%i,rgn:%i,ops:%i,bind:%i",
					bind->meta_idx, bind->region_idx, bind->ops_idx, bind->bind_cb_idx);
			}
			//else if( strncmp("internal_bind_ops ", str, 18) == 0 ) {
			//	// Get region using index
			//}
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
	driver_module->nRegions = 1+nSecondaryRgns;

	
	// Check for orphan drivers, and create an instance of them when loaded
	if( driver_module->nParents == 0 )
	{
		tUDI_DriverInstance *inst = UDI_CreateInstance(driver_module);
	
		// Enumerate so any pre-loaded drivers are detected	
		UDI_int_BeginEnumeration(inst);
	}
	else
	{
		// Send rescan requests to all loaded instances that support a parent metalang
	}

	return driver_module;
}

void UDI_int_DumpInitInfo(udi_init_t *info)
{
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
	Log("}");
	Log("ops_init_list = %p {", info->ops_init_list);
	for( int i = 0; info->ops_init_list[i].ops_idx; i++ )
	{
		Log(" [%i] = {", i);
		Log("  .ops_idx = 0x%x", info->ops_init_list[i].ops_idx);
		Log("  .meta_idx = 0x%x", info->ops_init_list[i].meta_idx);
		Log("  .meta_ops_num = 0x%x", info->ops_init_list[i].meta_ops_num);
		Log("  .chan_context_size = 0x%x", info->ops_init_list[i].chan_context_size);
		Log("  .ops_vector = %p", info->ops_init_list[i].ops_vector);
//		Log("  .op_flags = %p", info->ops_init_list[i].op_flags);
		Log(" }");
	}
	Log("}");
	Log("cb_init_list = %p {", info->cb_init_list);
	for( int i = 0; info->cb_init_list[i].cb_idx; i++ )
	{
		udi_cb_init_t	*ent = &info->cb_init_list[i];
		Log(" [%i] = {", i);
		Log("  .cbidx = %i", ent->cb_idx);
		Log("  .meta_idx = %i", ent->meta_idx);
		Log("  .meta_cb_num = %i", ent->meta_cb_num);
		Log("  .scratch_requirement = 0x%x", ent->scratch_requirement);
		Log("  .inline_size = 0x%x", ent->inline_size);
		Log("  .inline_layout = %p", ent->inline_layout);
		Log(" }");
	}
}

tUDI_DriverInstance *UDI_CreateInstance(tUDI_DriverModule *DriverModule)
{
	tUDI_DriverInstance	*inst = NEW_wA(tUDI_DriverInstance, Regions, DriverModule->nRegions);
	udi_primary_init_t	*pri_init = DriverModule->InitInfo->primary_init_info;
	inst->Module = DriverModule;
	inst->Regions[0] = UDI_InitRegion(inst, 0, 0, pri_init->rdata_size);
	udi_secondary_init_t	*sec_init = DriverModule->InitInfo->secondary_init_list;
	if( sec_init )
	{
		for( int i = 0; sec_init[i].region_idx; i ++ )
		{
			inst->Regions[1+i] = UDI_InitRegion(inst, i,
				sec_init[i].region_idx, sec_init[i].rdata_size);
		}
	}

	//inst->ManagementChannel = UDI_CreateChannel(METALANG_MGMT, 0, NULL, 0, inst, 0);
	inst->ManagementChannel = UDI_CreateChannel_Blank(&cMetaLang_Management);
	UDI_BindChannel_Raw(inst->ManagementChannel, true,
		0, inst->Regions[0]->InitContext, pri_init->mgmt_ops);
//	UDI_BindChannel_Raw(inst->ManagementChannel, false,
//		1, inst, NULL);	// TODO: ops list for management

	for( int i = 0; i < DriverModule->nRegions; i ++ )
		Log("Rgn %i: %p", i, inst->Regions[i]);

	// Send usage indication
	char scratch[pri_init->mgmt_scratch_requirement];
	{
		udi_usage_cb_t ucb;
		memset(&ucb, 0, sizeof(ucb));
		ucb.gcb.scratch = scratch;
		ucb.gcb.channel = inst->ManagementChannel;
		udi_usage_ind(&ucb, UDI_RESOURCES_NORMAL);
		// TODO: Ensure that udi_usage_res is called
	}
	
	for( int i = 1; i < DriverModule->nRegions; i ++ )
	{
		//inst->Regions[i]->PriChannel = UDI_CreateChannel_Blank(
		// TODO: Bind secondaries to primary
	}
	
	return inst;
}

tUDI_DriverRegion *UDI_InitRegion(tUDI_DriverInstance *Inst, udi_ubit16_t Index, udi_ubit16_t Type, size_t RDataSize)
{
//	ASSERTCR( RDataSize, <=, UDI_MIN_ALLOC_LIMIT, NULL );
	ASSERTCR( RDataSize, >=, sizeof(udi_init_context_t), NULL );
	tUDI_DriverRegion	*rgn = NEW(tUDI_DriverRegion,+RDataSize);
	rgn->InitContext = (void*)(rgn+1);
	rgn->InitContext->region_idx = Type;
//	rgn->InitContext->limits
	return rgn;
}

void UDI_int_BeginEnumeration(tUDI_DriverInstance *Inst)
{
	udi_primary_init_t *pri_init = Inst->Module->InitInfo->primary_init_info;
	char scratch[pri_init->mgmt_scratch_requirement];
	udi_enumerate_cb_t	ecb;
	memset(&ecb, 0, sizeof(ecb));
	ecb.gcb.scratch = scratch;
	ecb.gcb.channel = Inst->ManagementChannel;
	ecb.child_data = malloc( pri_init->child_data_size);
	ecb.attr_list = NEW(udi_instance_attr_list_t, *pri_init->enumeration_attr_list_length);
	ecb.attr_valid_length = 0;
	udi_enumerate_req(&ecb, UDI_ENUMERATE_START);
}

// TODO: Move this stuff out
udi_ops_init_t *UDI_int_GetOps(tUDI_DriverInstance *Inst, udi_index_t index)
{
	udi_ops_init_t *ops = Inst->Module->InitInfo->ops_init_list;
	while( ops->ops_idx && ops->ops_idx != index )
		ops ++;
	if(ops->ops_idx == 0)
		return NULL;
	return ops;
}

tUDI_MetaLang *UDI_int_GetMetaLang(tUDI_DriverInstance *Inst, udi_index_t index)
{
	if( index == 0 )
		return &cMetaLang_Management;
	for( int i = 0; i < Inst->Module->nMetaLangs; i ++ )
	{
		if( Inst->Module->MetaLangs[i].meta_idx == index )
			return Inst->Module->MetaLangs[i].metalang;
	}
	return NULL;
}

void *udi_cb_alloc_internal(tUDI_DriverInstance *Inst, udi_ubit8_t bind_cb_idx, udi_channel_t channel)
{
	udi_cb_init_t	*cb_init = NULL;
	for( cb_init = Inst->Module->InitInfo->cb_init_list; cb_init->cb_idx; cb_init ++ )
	{
		if( cb_init->cb_idx == bind_cb_idx )
		{
			udi_cb_t *ret = NEW(udi_cb_t, + cb_init->inline_size + cb_init->scratch_requirement);
			ret->channel = channel;
			return ret;
		}
	}
	Log_Warning("UDI", "Cannot find CB init def %i for '%s'",
		bind_cb_idx, Inst->Module->ModuleName);
	return NULL;
	
}

