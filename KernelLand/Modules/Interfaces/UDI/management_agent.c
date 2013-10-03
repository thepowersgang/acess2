/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 * 
 * management_agent.c
 * - Managment Agent
 */
#include <acess.h>
#include <udi.h>
#include "udi_internal.h"
#include "udi_ma.h"

// === CONSTANTS ===
enum {
	MGMT_CB_ENUM = 1,
};
const udi_cb_init_t cUDI_MgmtCbInitList[] = {
	{MGMT_CB_ENUM,0,0, 0,0,NULL},
	{0}
};

// === PROTOTYPES ===
tUDI_DriverInstance *UDI_MA_CreateChildInstance(tUDI_DriverModule *Module,
	tUDI_DriverInstance *ParentInstance, tUDI_ChildBinding *ChildBinding);

// === GLOBALS ===
tUDI_DriverInstance	*gpUDI_ActiveInstances;

// === CODE ===
tUDI_DriverInstance *UDI_MA_CreateInstance(tUDI_DriverModule *DriverModule)
{
	tUDI_DriverInstance	*inst = NEW_wA(tUDI_DriverInstance, Regions, DriverModule->nRegions);
	udi_primary_init_t	*pri_init = DriverModule->InitInfo->primary_init_info;
	inst->Module = DriverModule;
	inst->Regions[0] = UDI_MA_InitRegion(inst, 0, 0, pri_init->rdata_size);
	udi_secondary_init_t	*sec_init = DriverModule->InitInfo->secondary_init_list;
	if( sec_init )
	{
		for( int i = 0; sec_init[i].region_idx; i ++ )
		{
			inst->Regions[1+i] = UDI_MA_InitRegion(inst, i,
				sec_init[i].region_idx, sec_init[i].rdata_size);
		}
	}

	inst->ManagementChannel = UDI_CreateChannel_Blank(&cMetaLang_Management);
	UDI_BindChannel_Raw(inst->ManagementChannel, true,
		inst, 0, inst->Regions[0]->InitContext, pri_init->mgmt_ops);
//	UDI_BindChannel_Raw(inst->ManagementChannel, false,
//		NULL, 1, inst, &cUDI_MgmtOpsList);	// TODO: ops list for management agent?

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
		// TODO: Bind secondaries to primary
		Log_Warning("UDI", "TODO: Bind secondary channels");
		//inst->Regions[i]->PriChannel = UDI_CreateChannel_Blank(
	}

	// Add to global list of active instances
	inst->Next = gpUDI_ActiveInstances;
	gpUDI_ActiveInstances = inst;
	
	return inst;
}

tUDI_DriverRegion *UDI_MA_InitRegion(tUDI_DriverInstance *Inst,
	udi_ubit16_t Index, udi_ubit16_t Type, size_t RDataSize)
{
//	ASSERTCR( RDataSize, <=, UDI_MIN_ALLOC_LIMIT, NULL );
	ASSERTCR( RDataSize, >=, sizeof(udi_init_context_t), NULL );
	tUDI_DriverRegion	*rgn = NEW(tUDI_DriverRegion,+RDataSize);
	rgn->InitContext = (void*)(rgn+1);
	rgn->InitContext->region_idx = Type;
//	rgn->InitContext->limits
	return rgn;
}

void UDI_MA_BeginEnumeration(tUDI_DriverInstance *Inst)
{
	udi_primary_init_t *pri_init = Inst->Module->InitInfo->primary_init_info;
	udi_enumerate_cb_t	*ecb = udi_cb_alloc_internal(NULL, MGMT_CB_ENUM, Inst->ManagementChannel);
	memset(ecb, 0, sizeof(ecb));
	ecb->gcb.scratch = malloc(pri_init->mgmt_scratch_requirement);
	ecb->gcb.channel = Inst->ManagementChannel;
	ecb->child_data = malloc(pri_init->child_data_size);
	ecb->attr_list = NEW(udi_instance_attr_list_t, *pri_init->enumeration_attr_list_length);
	ecb->attr_valid_length = 0;
	udi_enumerate_req(ecb, UDI_ENUMERATE_START);
	Threads_Yield();	// Yield to allow udi_enumerate_req to run
}

/*
 * Returns number of matching attributes
 * If an attribute differs, returns 0
 */
int UDI_MA_CheckDeviceMatch(int nDevAttr, udi_instance_attr_list_t *DevAttrs,
	int nEnumAttr, udi_instance_attr_list_t *EnumAttrs)
{
	int n_matches = 0;
	for( int i = 0; i < nDevAttr; i ++ )
	{
		udi_instance_attr_list_t *dev_attr = &DevAttrs[i];
		udi_instance_attr_list_t *enum_attr = NULL;
		for( int j = 0; j < nEnumAttr; j ++ )
		{
			if( strcmp(dev_attr->attr_name, EnumAttrs[j].attr_name) == 0 ) {
				enum_attr = &EnumAttrs[j];
				break;
			}
		}
		if( enum_attr )
		{
			if( enum_attr->attr_length != dev_attr->attr_length )
				return 0;
			if( memcmp(enum_attr->attr_value, dev_attr->attr_value, dev_attr->attr_length) != 0 )
				return 0;
			n_matches ++;
		}
		else
		{
			// Attribute desired is missing, error?
		}
	}
	return n_matches;
}

void UDI_MA_AddChild(udi_enumerate_cb_t *cb, udi_index_t ops_idx)
{
	// Current side is MA, other is instance
	tUDI_DriverInstance *inst = UDI_int_ChannelGetInstance( UDI_GCB(cb), true );
	LOG("inst = %p", inst);
	
	// Search for existing child with same child_ID and ops
	for( tUDI_ChildBinding *child = inst->FirstChild; child; child = child->Next )
	{
		if( child->ChildID == cb->child_ID && child->Ops->ops_idx == ops_idx ) {
			LOG("duplicate, not creating");
			return;
		}
	}
	
	// Create a new child
	tUDI_ChildBinding *child = NEW_wA(tUDI_ChildBinding, Attribs, cb->attr_valid_length);
	child->Next = NULL;
	child->ChildID = cb->child_ID;
	child->Ops = UDI_int_GetOps(inst, ops_idx);
	child->BoundInstance = NULL;	// currently unbound
	child->BindOps = NULL;
	child->nAttribs = cb->attr_valid_length;
	memcpy(child->Attribs, cb->attr_list, sizeof(cb->attr_list[0])*cb->attr_valid_length);

	// - Locate child_bind_ops definition
	for( int i = 0; i < inst->Module->nChildBindOps; i ++ )
	{
		LOG(" %i == %i?", inst->Module->ChildBindOps[i].ops_idx, ops_idx);
		if( inst->Module->ChildBindOps[i].ops_idx == ops_idx ) {
			child->BindOps = &inst->Module->ChildBindOps[i];
			break;
		}
	}
	if( !child->BindOps ) {
		Log_Error("UDI", "Driver '%s' doesn't have a 'child_bind_ops' for %i",
			inst->Module->ModuleName, ops_idx);
		free(child);
		return ;
	}

	child->Next = inst->FirstChild;
	inst->FirstChild = child;
	
	// and search for a handler
	tUDI_MetaLang	*metalang = UDI_int_GetMetaLang(inst, child->Ops->meta_idx);
	 int	best_level = 0;
	tUDI_DriverModule *best_module = NULL;
	for( tUDI_DriverModule *module = gpUDI_LoadedModules; module; module = module->Next )
	{
		for( int i = 0; i < module->nDevices; i ++ )
		{
			if( module->Devices[i]->Metalang != metalang )
				continue ;
			
			int level = UDI_MA_CheckDeviceMatch(
				module->Devices[i]->nAttribs, module->Devices[i]->Attribs,
				child->nAttribs, child->Attribs
				);
			if( level > best_level ) {
				best_level = level;
				best_module = module;
			}
		}
	}
	if( best_module != NULL )
	{
		UDI_MA_CreateChildInstance(best_module, inst, child);
	}
}

void UDI_MA_BindParents(tUDI_DriverModule *Module)
{
	UNIMPLEMENTED();
}

tUDI_DriverInstance *UDI_MA_CreateChildInstance(tUDI_DriverModule *Module,
	tUDI_DriverInstance *ParentInstance, tUDI_ChildBinding *ChildBinding)
{
	// Found a match, so create an instance and bind it
	tUDI_DriverInstance *inst = UDI_MA_CreateInstance(Module);
	ChildBinding->BoundInstance = inst;
	
	// TODO: Handle multi-parent drivers
	ASSERTC(Module->nParents, ==, 1);
	
	// Bind to parent
	tUDI_BindOps	*parent = &Module->Parents[0];
	udi_channel_t channel = UDI_CreateChannel_Blank( UDI_int_GetMetaLang(inst, parent->meta_idx) );
	
	UDI_BindChannel(channel,true,  inst, parent->ops_idx, parent->region_idx);
	UDI_BindChannel(channel,false,
		ParentInstance, ChildBinding->Ops->ops_idx, ChildBinding->BindOps->region_idx);
	
	udi_cb_t *bind_cb = udi_cb_alloc_internal(inst, parent->bind_cb_idx, channel);
	if( !bind_cb ) {
		Log_Warning("UDI", "Bind CB index is invalid");
		return NULL;
	}

	udi_channel_event_cb_t	ev_cb;
	 int	n_handles = Module->InitInfo->primary_init_info->per_parent_paths;
	udi_buf_path_t	handles[n_handles];
	memset(&ev_cb, 0, sizeof(ev_cb));
	ev_cb.gcb.channel = channel;
	ev_cb.event = UDI_CHANNEL_BOUND;
	ev_cb.params.parent_bound.bind_cb = bind_cb;
	ev_cb.params.parent_bound.parent_ID = 1;
	ev_cb.params.parent_bound.path_handles = handles;
	
	for( int i = 0; i < n_handles; i ++ ) {
		//handles[i] = udi_buf_path_alloc_internal(inst);
		handles[i] = 0;
	}
	
	udi_channel_event_ind(&ev_cb);
	return inst;
}

