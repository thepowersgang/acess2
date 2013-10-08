/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - UDI Entrypoint and Module loading
 */
#define DEBUG	1
#define VERSION	((0<<8)|1)
#include <acess.h>
#include <modules.h>
#include <udi.h>
#include <udi_internal.h>
#include <udi_internal_ma.h>
#include <trans_pci.h>

// === PROTOTYPES ===
 int	UDI_Install(char **Arguments);
 int	UDI_DetectDriver(void *Base);
 int	UDI_LoadDriver(void *Base);
tUDI_DriverModule	*UDI_int_LoadDriver(void *LoadBase, udi_init_t *info, const char *udiprops, size_t udiprops_size);
const tUDI_MetaLang	*UDI_int_GetMetaLangByName(const char *Name);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, UDI, UDI_Install, NULL, NULL);
tModuleLoader	gUDI_Loader = {
	NULL, "UDI", UDI_DetectDriver, UDI_LoadDriver, NULL
};
tUDI_DriverModule	*gpUDI_LoadedModules;

// === CODE ===
/**
 * \fn int UDI_Install(char **Arguments)
 * \brief Stub intialisation routine
 */
int UDI_Install(char **Arguments)
{
	Module_RegisterLoader( &gUDI_Loader );

	Proc_SpawnWorker(UDI_int_DeferredThread, NULL);

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
	
	return 0;
}

static udi_boolean_t _get_token_bool(const char *str, const char **outstr)
{
	udi_boolean_t	ret;
	switch(*str++)
	{
	case 't':
	case 'T':
		ret = 1;
		break;
	case 'f':
	case 'F':
		ret = 0;
		break;
	default:
		*outstr = NULL;
		return 0;
	}
	while( isspace(*str) )
		str ++;
	*outstr = str;
	return ret;
}
static udi_index_t _get_token_idx(const char *str, const char **outstr)
{
	char	*end;
	int ret = strtol(str, &end, 10);
	if( ret < 0 || ret > 255 ) {
		Log_Notice("UDI", "Value '%.*s' out of range for udi_index_t",
			end-str, str);
		*outstr = NULL;
		return 0;
	}
	if( *end && !isspace(*end) ) {
		Log_Notice("UDI", "No whitespace following '%.*s', got '%c'",
			end-str, str, *end);
		*outstr = NULL;
		return 0;
	}
	while( *end && isspace(*end) )
		end ++;
	
	*outstr = end;
	return ret;
}
static udi_ubit16_t _get_token_uint16(const char *str, const char **outstr)
{
	char	*end;
	unsigned long ret = strtoul(str, &end, 10);
	if( ret > 0xFFFF ) {
		Log_Notice("UDI", "Value '%.*s' out of range for udi_ubit16_t",
			end-str, str);
		*outstr = NULL;
		return 0;
	}
	if( *end && !isspace(*end) ) {
		Log_Notice("UDI", "No whitespace following '%.*s', got '%c'",
			end-str, str, *end);
		*outstr = NULL;
		return 0;
	}
	while( *end && isspace(*end) )
		end ++;
	
	*outstr = end;
	return ret;
}
static udi_ubit32_t _get_token_uint32(const char *str, const char **outstr)
{
	char	*end;
	udi_ubit32_t ret = strtoul(str, &end, 0);
	if( *end && !isspace(*end) ) {
		Log_Notice("UDI", "No whitespace following '%.*s', got '%c'",
			end-str, str, *end);
		*outstr = NULL;
		return 0;
	}
	while( *end && isspace(*end) )
		end ++;
	
	*outstr = end;
	return ret;
}
static size_t _get_token_str(const char *str, const char **outstr, char *output)
{
	size_t	ret = 0;
	const char *pos = str;
	while( *pos && !isspace(*pos) )
	{
		if( *pos == '\\' )
		{
			pos ++;
			switch( *pos )
			{
			case '_':	// space
				if(output)
					output[ret] = ' ';
				ret ++;
				break;
			case 'H':	// hash
				if(output)
					output[ret] = '#';
				ret ++;
				break;
			case '\\':	// backslash
				if(output)
					output[ret] = '\\';
				ret ++;
				break;
			// TODO: \p and \m<msgnum> (for message/disaster_message only)
			default:
				// Error
				break;
			}
		}
		else {
			if(output)
				output[ret] = *pos;
			ret ++;
		}
		pos ++;
	}

	while( isspace(*pos) )
		pos ++;
	*outstr = pos;	

	if(output)
		output[ret] = '\0';

	return ret;
}
static int _get_token_sym_v(const char *str, const char **outstr, bool printError, const char **syms)
{
	const char *sym;
	for( int idx = 0; (sym = syms[idx]); idx ++ )
	{
		size_t len = strlen(sym);
		if( memcmp(str, sym, len) != 0 )
			continue ;
		if( str[len] && !isspace(str[len]) )
			continue ;
		
		// Found it!
		str += len;
		while( isspace(*str) )
			str ++;
		*outstr = str;
		return idx;
	}

	// Unknown symbol, find the end of the symbol and error
	const char *end = str;
	while( !isspace(*end) )
		end ++;
	
	if( printError ) {
		Log_Notice("UDI", "Unknown token '%.*s'", end-str, str);
	}

	*outstr = NULL;
	return -1;
	
}
static int _get_token_sym(const char *str, const char **outstr, bool printError, ...)
{
	va_list args;
	const char *sym;
	 int	count = 0;
	va_start(args, printError);
	for( ; (sym = va_arg(args, const char *)); count ++ )
		;
	va_end(args);

	const char	*symlist[count+1];	
	va_start(args, printError);
	for( int idx = 0; (sym = va_arg(args, const char *)); idx ++ )
		symlist[idx] = sym;
	symlist[count] = NULL;
	
	return _get_token_sym_v(str, outstr, printError, symlist);
}

enum {
	UDIPROPS__properties_version,
	UDIPROPS__module,
	UDIPROPS__meta,
	UDIPROPS__message,
	UDIPROPS__locale,
	UDIPROPS__region,
	UDIPROPS__parent_bind_ops,
	UDIPROPS__internal_bind_ops,
	UDIPROPS__child_bind_ops,
	UDIPROPS__supplier,
	UDIPROPS__contact,
	UDIPROPS__name,
	UDIPROPS__shortname,
	UDIPROPS__release,
	
	UDIPROPS__requires,
	UDIPROPS__device,
	UDIPROPS__enumerates,

	UDIPROPS_last
};
#define _defpropname(name)	[ UDIPROPS__##name ] = #name
const char *caUDI_UdipropsNames[] = {
	_defpropname(properties_version),
	_defpropname(module),
	_defpropname(meta),
	_defpropname(message),
	_defpropname(locale),
	_defpropname(region),
	_defpropname(parent_bind_ops),
	_defpropname(internal_bind_ops),
	_defpropname(child_bind_ops),
	_defpropname(supplier),
	_defpropname(contact),
	_defpropname(name),
	_defpropname(shortname),
	_defpropname(release),
	_defpropname(requires),
	
	_defpropname(device),
	_defpropname(enumerates),
	
	[UDIPROPS_last] = NULL
};
#undef _defpropname

tUDI_DriverModule *UDI_int_LoadDriver(void *LoadBase, udi_init_t *info, const char *udiprops, size_t udiprops_size)
{
	//UDI_int_DumpInitInfo(info);
	
	// TODO: Multiple modules?
	tUDI_DriverModule *driver_module = NEW(tUDI_DriverModule,);
	driver_module->InitInfo = info;

	// - Parse udiprops
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
	
	// Parse out:
	// 'message' into driver_module->Messages
	// 'region' into driver_module->RegionTypes
	// 'module' into driver_module->ModuleName
	
	 int	nLocales = 1;
	for( int i = 0; i < nLines; i ++ )
	{
		const char *str = udipropsptrs[i];
		 int	sym = _get_token_sym_v(str, &str, false, caUDI_UdipropsNames);
		switch(sym)
		{
		case UDIPROPS__module:
			driver_module->ModuleName = str;
			break;
		case UDIPROPS__meta:
			driver_module->nMetaLangs ++;
			break;
		case UDIPROPS__message:
			driver_module->nMessages ++;
			break;
		case UDIPROPS__locale:
			nLocales ++;
			break;
		case UDIPROPS__region:
			driver_module->nRegionTypes ++;
			break;
		case UDIPROPS__device:
			driver_module->nDevices ++;
			break;
		case UDIPROPS__parent_bind_ops:
			driver_module->nParents ++;
			break;
		case UDIPROPS__child_bind_ops:
			driver_module->nChildBindOps ++;
			break;
		default:
			// quiet ignore
			break;
		}
	}

	// Allocate structures
	LOG("nMessages = %i, nMetaLangs = %i",
		driver_module->nMessages,
		driver_module->nMetaLangs);
	driver_module->Messages     = NEW(tUDI_PropMessage, * driver_module->nMessages);
	driver_module->RegionTypes  = NEW(tUDI_PropRegion,  * driver_module->nRegionTypes);
	driver_module->MetaLangs    = NEW(tUDI_MetaLangRef, * driver_module->nMetaLangs);
	driver_module->Parents      = NEW(tUDI_BindOps,     * driver_module->nParents);
	driver_module->ChildBindOps = NEW(tUDI_BindOps,     * driver_module->nChildBindOps);
	driver_module->Devices      = NEW(tUDI_PropDevSpec*,* driver_module->nDevices);

	// Populate
	 int	cur_locale = 0;
	 int	msg_index = 0;
	 int	ml_index = 0;
	 int	parent_index = 0;
	 int	child_bind_index = 0;
	 int	device_index = 0;
	 int	next_unpop_region = 1;
	for( int i = 0; i < nLines; i ++ )
	{
		const char *str = udipropsptrs[i];
		if( !*str )
			continue ;
		 int	sym = _get_token_sym_v(str, &str, true, caUDI_UdipropsNames);
		switch(sym)
		{
		case UDIPROPS__properties_version:
			if( _get_token_uint32(str, &str) != 0x101 ) {
				Log_Warning("UDI", "Properties version mismatch.");
			}
			break;
		case UDIPROPS__module:
			driver_module->ModuleName = str;
			break;
		case UDIPROPS__meta:
			{
			tUDI_MetaLangRef *ml = &driver_module->MetaLangs[ml_index++];
			ml->meta_idx = _get_token_idx(str, &str);
			if( !str )	continue;
			ml->interface_name = str;
			// TODO: May need to trim trailing spaces
			ml->metalang = UDI_int_GetMetaLangByName(ml->interface_name);
			if( !ml->metalang ) {
				Log_Error("UDI", "Module %s referenced unsupported metalang %s",
					driver_module->ModuleName, ml->interface_name);
			}
			break;
			}
		case UDIPROPS__message:
			{
			tUDI_PropMessage *msg = &driver_module->Messages[msg_index++];
			msg->locale = cur_locale;
			msg->index = _get_token_uint16(str, &str);
			if( !str )	continue ;
			msg->string = str;
			//Log_Debug("UDI", "Message %i/%i: '%s'", msg->locale, msg->index, msg->string);
			break;
			}
		case UDIPROPS__locale:
			// TODO: Set locale
			cur_locale = 1;
			break;
		case UDIPROPS__region:
			{
			udi_index_t rgn_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			// Search for region index (just in case internal_bind_ops appears earlier)
			tUDI_PropRegion	*rgn = &driver_module->RegionTypes[0];
			if( rgn_idx > 0 )
			{
				rgn ++;
				for( int i = 1; i < next_unpop_region; i ++, rgn ++ ) {
					if( rgn->RegionIdx == rgn_idx )
						break;
				}
				if(i == next_unpop_region) {
					if( next_unpop_region == driver_module->nRegionTypes ) {
						// TODO: warning if reigon types overflow
						continue ;
					}
					next_unpop_region ++;
					rgn->RegionIdx = rgn_idx;
				}
			}
			// Parse attributes
			while( *str )
			{
				int sym = _get_token_sym(str, &str, true,
					"type", "binding", "priority", "latency", "overrun_time", NULL
					);
				if( !str )	break ;
				switch(sym)
				{
				case 0:	// type
					rgn->Type = _get_token_sym(str, &str, true,
						"normal", "fp", NULL);
					break;
				case 1:	// binding
					rgn->Binding = _get_token_sym(str, &str, true,
						"static", "dynamic", NULL);
					break;
				case 2:	// priority
					rgn->Priority = _get_token_sym(str, &str, true,
						"med", "lo", "hi", NULL);
					break;
				case 3:	// latency
					rgn->Latency = _get_token_sym(str, &str, true,
						"non_overrunable", "powerfail_warning", "overrunable",
						"retryable", "non_critical", NULL);
					break;
				case 4:	// overrun_time
					rgn->OverrunTime = _get_token_uint32(str, &str);
					break;
				}
				if( !str )	break ;
			}
			break;
			}
		case UDIPROPS__parent_bind_ops:
			{
			tUDI_BindOps	*bind = &driver_module->Parents[parent_index++];
			bind->meta_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			bind->region_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			bind->ops_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			bind->bind_cb_idx = _get_token_idx(str, &str);
			if( *str ) {
				// Expected EOL, didn't get it :(
			}
			Log_Debug("UDI", "Parent bind - meta:%i,rgn:%i,ops:%i,bind:%i",
				bind->meta_idx, bind->region_idx, bind->ops_idx, bind->bind_cb_idx);
			break;
			}
		case UDIPROPS__internal_bind_ops:
			{
			// Get region using index
			udi_index_t meta = _get_token_idx(str, &str);
			if( !str )	continue ;
			udi_index_t rgn_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			
			// Search for region index (just in case the relevant 'region' comes after)
			tUDI_PropRegion	*rgn = &driver_module->RegionTypes[0];
			if( rgn_idx > 0 )
			{
				rgn ++;
				 int	j;
				for( j = 1; j < next_unpop_region; j ++, rgn ++ ) {
					if( rgn->RegionIdx == rgn_idx )
						break;
				}
				if( j == next_unpop_region ) {
					if( next_unpop_region == driver_module->nRegionTypes ) {
						// TODO: warning if reigon types overflow
						continue ;
					}
					next_unpop_region ++;
					rgn->RegionIdx = rgn_idx;
				}
			}
	
			// Set properties
			rgn->BindMeta = meta;
			
			rgn->PriBindOps = _get_token_idx(str, &str);
			if( !str )	continue ;
			rgn->SecBindOps = _get_token_idx(str, &str);
			if( !str )	continue ;
			rgn->BindCb = _get_token_idx(str, &str);
			if( !str )	continue ;
			if( *str ) {
				// TODO: Please sir, I want an EOL
			}
			break;
			}
		case UDIPROPS__child_bind_ops:
			{
			tUDI_BindOps	*bind = &driver_module->ChildBindOps[child_bind_index++];
			bind->meta_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			bind->region_idx = _get_token_idx(str, &str);
			if( !str )	continue ;
			bind->ops_idx = _get_token_idx(str, &str);
			if( *str ) {
				// Expected EOL, didn't get it :(
			}
			Log_Debug("UDI", "Child bind - meta:%i,rgn:%i,ops:%i",
				bind->meta_idx, bind->region_idx, bind->ops_idx);
			break;
			}
		case UDIPROPS__supplier:
		case UDIPROPS__contact:
		case UDIPROPS__name:
		case UDIPROPS__shortname:
		case UDIPROPS__release:
			break;
		//case UDIPROPS__requires:
		//	// TODO: Requires
		//	break;
		case UDIPROPS__device:
			{
			 int	n_attr = 0;
			// Count properties (and validate)
			_get_token_idx(str, &str);	// message
			if( !str )	continue;
			_get_token_idx(str, &str);	// meta
			if( !str )	continue;
			while( *str )
			{
				_get_token_str(str, &str, NULL);
				if( !str )	break;
				_get_token_sym(str, &str, true, "string", "ubit32", "boolean", "array", NULL);
				if( !str )	break;
				_get_token_str(str, &str, NULL);
				if( !str )	break;
				n_attr ++;
			}
			// Rewind and actually parse
			_get_token_str(udipropsptrs[i], &str, NULL);
			
			tUDI_PropDevSpec *dev = NEW_wA(tUDI_PropDevSpec, Attribs, n_attr);
			driver_module->Devices[device_index++] = dev;;
			dev->MessageNum = _get_token_idx(str, &str);
			dev->MetaIdx = _get_token_idx(str, &str);
			dev->nAttribs = n_attr;
			n_attr = 0;
			while( *str )
			{
				udi_instance_attr_list_t *at = &dev->Attribs[n_attr];
				_get_token_str(str, &str, at->attr_name);
				if( !str )	break;
				at->attr_type = _get_token_sym(str, &str, true,
					" ", "string", "array", "ubit32", "boolean", NULL);
				if( !str )	break;
				udi_ubit32_t	val;
				switch( dev->Attribs[n_attr].attr_type )
				{
				case 1:	// String
					at->attr_length = _get_token_str(str, &str, (char*)at->attr_value);
					break;
				case 2:	// Array
					// TODO: Array
					Log_Warning("UDI", "TODO: Parse 'array' attribute in 'device'");
					_get_token_str(str, &str, NULL);
					break;
				case 3:	// ubit32
					at->attr_length = sizeof(udi_ubit32_t);
					val = _get_token_uint32(str, &str);
					Log_Debug("UDI", "device %i: Value '%s'=%x", device_index,
						at->attr_name, val);
					UDI_ATTR32_SET(at->attr_value, val);
					break;
				case 4:	// boolean
					at->attr_length = sizeof(udi_boolean_t);
					UDI_ATTR32_SET(at->attr_value, _get_token_bool(str, &str));
					break;
				}
				if( !str )	break;
				n_attr ++;
			}
			
			break;
			}
		default:
			Log_Debug("UDI", "udipropsptrs[%i] = '%s'", i, udipropsptrs[i]);
			break;
		}
	}
	free(udipropsptrs);
	
	// Sort message list
	// TODO: Sort message list

	 int	nSecondaryRgns = 0;
	for( int i = 0; info->secondary_init_list && info->secondary_init_list[i].region_idx; i ++ )
		nSecondaryRgns ++;
	driver_module->nRegions = 1+nSecondaryRgns;

	
	// -- Add to loaded module list
	driver_module->Next = gpUDI_LoadedModules;
	gpUDI_LoadedModules = driver_module;
	
	// Check for orphan drivers, and create an instance of them when loaded
	if( driver_module->nParents == 0 )
	{
		UDI_MA_CreateInstance(driver_module, NULL, NULL);
	}
	else
	{
		// Search running instances for unbound children that can be bound to this driver
		UDI_MA_BindParents(driver_module);
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
	ASSERT(Inst);
	for( int i = 0; i < Inst->Module->nMetaLangs; i ++ )
	{
		if( Inst->Module->MetaLangs[i].meta_idx == index )
			return Inst->Module->MetaLangs[i].metalang;
	}
	return NULL;
}

const tUDI_MetaLang *UDI_int_GetMetaLangByName(const char *Name)
{
	//extern tUDI_MetaLang	cMetaLang_Management;
	extern tUDI_MetaLang	cMetaLang_BusBridge;
	const tUDI_MetaLang	*langs[] = {
		&cMetaLang_BusBridge,
		NULL
	};
	for( int i = 0; langs[i]; i ++ )
	{
		if( strcmp(Name, langs[i]->Name) == 0 )
			return langs[i];
	}
	return NULL;
}

