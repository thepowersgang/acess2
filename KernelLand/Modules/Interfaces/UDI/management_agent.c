/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 * 
 * management_agent.c
 * - Managment Agent
 */
#define DEBUG	1
#include <udi.h>
#include <acess.h>
#include <udi_internal.h>
#include <udi_internal_ma.h>

// === CONSTANTS ===

// === PROTOTYPES ===

// === GLOBALS ===
tUDI_DriverInstance	*gpUDI_ActiveInstances;

// === CODE ===
tUDI_DriverInstance *UDI_MA_CreateInstance(tUDI_DriverModule *DriverModule,
	tUDI_DriverInstance *ParentInstance, tUDI_ChildBinding *ChildBinding)
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

	if( ParentInstance ) {
		ASSERT(ChildBinding);
		ChildBinding->BoundInstance = inst;
	}
	inst->Parent = ParentInstance;
	inst->ParentChildBinding = ChildBinding;

	inst->ManagementChannel = UDI_CreateChannel_Blank(&cMetaLang_Management);
	UDI_BindChannel_Raw(inst->ManagementChannel, true,
		inst, 0, 0, inst->Regions[0]->InitContext, pri_init->mgmt_ops);
//	UDI_BindChannel_Raw(inst->ManagementChannel, false,
//		NULL, 1, inst, &cUDI_MgmtOpsList);	// TODO: ops list for management agent?

//	for( int i = 0; i < DriverModule->nRegions; i ++ )
//		Log("Rgn %i: %p", i, inst->Regions[i]);

	LOG("Inst %s %p MA state =%i",
		inst->Module->ModuleName, inst, UDI_MASTATE_USAGEIND);
	inst->CurState = UDI_MASTATE_USAGEIND;
	// Next State: _SECBIND

	// Add to global list of active instances
	inst->Next = gpUDI_ActiveInstances;
	gpUDI_ActiveInstances = inst;

	// Send usage indication
	udi_usage_cb_t *cb = (void*)udi_cb_alloc_internal_v(&cMetaLang_Management, UDI_MGMT_USAGE_CB_NUM,
		0, pri_init->mgmt_scratch_requirement, inst->ManagementChannel
		);
	UDI_GCB(cb)->initiator_context = inst;
	udi_usage_ind(cb, UDI_RESOURCES_NORMAL);
	// udi_usage_res causes next state transition
	
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
	udi_enumerate_cb_t	*ecb = (void*)udi_cb_alloc_internal_v(
		&cMetaLang_Management, UDI_MGMT_ENUMERATE_CB_NUM,
		0, pri_init->mgmt_scratch_requirement, Inst->ManagementChannel);
	UDI_GCB(ecb)->initiator_context = Inst;
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
	// TODO: Ask metalangauge instead
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
			//LOG("Match = '%s' (%i %x == %i %x)",
			//	dev_attr->attr_name,
			//	dev_attr->attr_length, UDI_ATTR32_GET(dev_attr->attr_value),
			//	enum_attr->attr_length, UDI_ATTR32_GET(enum_attr->attr_value)
			//	);
			if( enum_attr->attr_length != dev_attr->attr_length )
				return 0;
			if( memcmp(enum_attr->attr_value, dev_attr->attr_value, dev_attr->attr_length) != 0 )
				return 0;
			n_matches ++;
		}
		else
		{
			// Attribute desired is missing, error?
			//LOG("attr '%s' missing", dev_attr->attr_name);
		}
	}
	//LOG("n_matches = %i", n_matches);
	return n_matches;
}

void UDI_MA_AddChild(udi_enumerate_cb_t *cb, udi_index_t ops_idx)
{
	// Current side is MA, other is instance
	// TODO: Get region index too?
	tUDI_DriverInstance *inst = UDI_int_ChannelGetInstance( UDI_GCB(cb), true, NULL );
	//LOG("inst = %p", inst);
	
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
	child->Metalang = UDI_int_GetMetaLang(inst->Module, child->Ops->meta_idx);
	 int	best_level = 0;
	tUDI_DriverModule *best_module = NULL;
	for( tUDI_DriverModule *module = gpUDI_LoadedModules; module; module = module->Next )
	{
		for( int i = 0; i < module->nDevices; i ++ )
		{
			//LOG("%s:%i %p ?== %p",
			//	module->ModuleName, i,
			//	module->Devices[i]->Metalang, metalang);
			if( module->Devices[i]->Metalang != child->Metalang )
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
		UDI_MA_CreateInstance(best_module, inst, child);
	}
}

void UDI_MA_BindParents(tUDI_DriverModule *Module)
{
	// Scan active instances for enumerated children that can be handled by this module
	for( int i = 0; i < Module->nDevices; i ++ )
	{
		// TODO: Have list of unbound enumerated children
		for( tUDI_DriverInstance *inst = gpUDI_ActiveInstances; inst; inst = inst->Next )
		{
			// Loop children
			for( tUDI_ChildBinding *child = inst->FirstChild; child; child = child->Next )
			{
				if( child->BoundInstance )
					continue ;
				if( Module->Devices[i]->Metalang != child->Metalang )
					continue ;
				// Check for match
				int level = UDI_MA_CheckDeviceMatch(
					Module->Devices[i]->nAttribs, Module->Devices[i]->Attribs,
					child->nAttribs, child->Attribs
					);
				// No match: Continue
				if( level == 0 )
					continue ;
				// Found a match, so create an instance (binding happens async)
				UDI_MA_CreateInstance(Module, inst, child);
			}
		}
	}
}

void UDI_MA_TransitionState(tUDI_DriverInstance *Inst, enum eUDI_MAState Src, enum eUDI_MAState Dst)
{
	ASSERT(Inst);
	if( Inst->CurState != Src )
		return ;
	
	LOG("Inst %s %p MA state %i->%i",
		Inst->Module->ModuleName, Inst, Src, Dst);
	
	switch(Dst)
	{
	case UDI_MASTATE_USAGEIND:
		ASSERT(Dst != UDI_MASTATE_USAGEIND);
		break;
	case UDI_MASTATE_SECBIND:
		Inst->CurState = UDI_MASTATE_SECBIND;
		for( int i = 1; i < Inst->Module->nRegions; i ++ )
		{
			// TODO: Bind secondaries to primary
			Log_Warning("UDI", "TODO: Bind secondary channels");
			//inst->Regions[i]->PriChannel = UDI_CreateChannel_Blank(
		}
		//UDI_MA_TransitionState(Inst, UDI_MASTATE_SECBIND, UDI_MASTATE_PARENTBIND);
		//break;
	case UDI_MASTATE_PARENTBIND:
		Inst->CurState = UDI_MASTATE_PARENTBIND;
		if( Inst->Parent )
		{
			tUDI_DriverModule	*Module = Inst->Module;
			tUDI_ChildBinding	*parent_bind = Inst->ParentChildBinding;
			// TODO: Handle multi-parent drivers
			ASSERTC(Module->nParents, ==, 1);
			
			// Bind to parent
			tUDI_BindOps	*parent = &Module->Parents[0];
			udi_channel_t channel = UDI_CreateChannel_Blank(UDI_int_GetMetaLang(Inst->Module, parent->meta_idx));
			
			UDI_BindChannel(channel,true,  Inst, parent->ops_idx, parent->region_idx, NULL,false,0);
			UDI_BindChannel(channel,false,
				Inst->Parent, parent_bind->Ops->ops_idx, parent_bind->BindOps->region_idx,
				NULL, true, parent_bind->ChildID);

			udi_cb_t	*bind_cb;
			if( parent->bind_cb_idx == 0 )
				bind_cb = NULL;
			else {
				bind_cb = udi_cb_alloc_internal(Inst, parent->bind_cb_idx, channel);
				if( !bind_cb ) {
					Log_Warning("UDI", "Bind CB index is invalid");
					break;
				}
				UDI_int_ChannelFlip( bind_cb );
			}

			 int	n_handles = Module->InitInfo->primary_init_info->per_parent_paths;
			udi_buf_path_t	handles[n_handles];
			for( int i = 0; i < n_handles; i ++ ) {
				//handles[i] = udi_buf_path_alloc_internal(Inst);
				handles[i] = 0;
			}
			
			udi_channel_event_cb_t *ev_cb = (void*)udi_cb_alloc_internal_v(&cMetaLang_Management,
				UDI_MGMT_CHANNEL_EVENT_CB_NUM, 0, 0, channel);
			UDI_GCB(ev_cb)->initiator_context = Inst;
			ev_cb->event = UDI_CHANNEL_BOUND;
			ev_cb->params.parent_bound.bind_cb = bind_cb;
			ev_cb->params.parent_bound.parent_ID = 1;
			ev_cb->params.parent_bound.path_handles = handles;
			
			udi_channel_event_ind(ev_cb);
			break;
		}
		//UDI_MA_TransitionState(Inst, UDI_MASTATE_PARENTBIND, UDI_MASTATE_ENUMCHILDREN);
		//break;
	case UDI_MASTATE_ENUMCHILDREN:
		Inst->CurState = UDI_MASTATE_ENUMCHILDREN;
		UDI_MA_BeginEnumeration(Inst);
		break;
	case UDI_MASTATE_ACTIVE:
		Inst->CurState = UDI_MASTATE_ACTIVE;
		Log_Log("UDI", "Driver instance %s %p entered active state",
			Inst->Module->ModuleName, Inst);
		break;
	}
}

