/*
 * Acess2 UDI Layer
 */
#define DEBUG	0
#define VERSION	((0<<8)|1)
#include <acess.h>
#include <modules.h>
#include <udi.h>

// === PROTOTYPES ===
 int	UDI_Install(char **Arguments);
 int	UDI_DetectDriver(void *Base);
 int	UDI_LoadDriver(void *Base);

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
	if( Binary_FindSymbol(Base, "udi_init_info", NULL) == 0) {
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
	 int	udiprops_size = 0;
	 int	i;
	// int	j;
	
	Log_Debug("UDI", "UDI_LoadDriver: (Base=%p)", Base);
	
	if( Binary_FindSymbol(Base, "udi_init_info", (Uint*)&info) == 0) {
		Binary_Unload(Base);
		return 0;
	}
	
	if( Binary_FindSymbol(Base, "_udiprops", (Uint*)&udiprops) == 0 ) {
		Log_Warning("UDI", "_udiprops is not defined, this is usually bad");
	}
	else {
		if( Binary_FindSymbol(Base, "_udiprops_size", (Uint*)&udiprops_size) == 0)
			Log_Warning("UDI", "_udiprops_size is not defined");
		Log_Log("UDI", "udiprops = %p, udiprops_size = 0x%x", udiprops, udiprops_size);
	}
	
	
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
	Log("secondary_init_list = %p", info->secondary_init_list);
	Log("ops_init_list = %p", info->ops_init_list);
	
	for( i = 0; info->ops_init_list[i].ops_idx; i++ )
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
	
	return 0;
}
