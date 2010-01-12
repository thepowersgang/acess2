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
 int	UDI_LoadDriver(void *Base);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, UDI, UDI_Install, NULL, NULL);

// === CODE ===
/**
 * \fn int UDI_Install(char **Arguments)
 * \brief Stub intialisation routine
 */
int UDI_Install(char **Arguments)
{
	return 1;
}

/**
 * \fn int UDI_LoadDriver(void *Base)
 */
int UDI_LoadDriver(void *Base)
{
	udi_init_t	*info;
	char	*udiprops = NULL;
	 int	udiprops_size;
	 int	i, j;
	
	if( Binary_FindSymbol(Base, "udi_init_info", (Uint*)&info) == 0) {
		Binary_Unload(Base);
		return 0;
	}
	
	if( Binary_FindSymbol(Base, "_udiprops", (Uint*)&udiprops) == 0 ) {
		Warning("[UDI  ] _udiprops is not defined, this is usually bad");
	}
	else {
		Binary_FindSymbol(Base, "_udiprops_size", (Uint*)&udiprops_size);
	}
	
	Log("primary_init_info = %p", info->primary_init_info);
	Log("secondary_init_list = %p", info->secondary_init_list);
	Log("ops_init_list = %p", info->ops_init_list);
	
	for( i = 0; info->ops_init_list[i].ops_idx; i++ )
	{
		Log("info->ops_init_list[%i] = {", i);
		Log(" .ops_idx = %i", info->ops_init_list[i].ops_idx);
		Log(" .meta_idx = %i", info->ops_init_list[i].meta_idx);
		Log(" .meta_ops_num = %i", info->ops_init_list[i].meta_ops_num);
		Log(" .chan_context_size = %i", info->ops_init_list[i].chan_context_size);
		Log(" .ops_vector = {");
		for( j = 0; info->ops_init_list[i].ops_vector; j++ )
		{
			Log("%i: %p()", j, info->ops_init_list[i].ops_vector);
		}
		Log(" }");
		Log(" .op_flags = %p", info->ops_init_list[i].op_flags);
		Log("}");
	}
	
	return 0;
}
