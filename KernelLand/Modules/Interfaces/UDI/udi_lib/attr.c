/**
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_lib/attr.c
 * - Instance Attribute Management
 */
#include <acess.h>
#include <udi.h>
#include "../udi_internal.h"

// Notes:
// - Prefixes:
//  > '%': Private persistent
//  > '$': Private Volatile
//  > '':  Enumeration
//  > '^': Sibling group
//  > '@': parent-visible (persistent)

// === CODE ===
udi_instance_attr_type_t udi_instance_attr_get_internal(udi_cb_t *gcb, const char *attr_name, udi_ubit32_t child_ID, void *attr_value, udi_size_t attr_length, udi_size_t *actual_length)
{
	// Get instance
	tUDI_DriverInstance *inst = UDI_int_ChannelGetInstance(gcb, false, NULL);
	
	const tUDI_ChildBinding *bind = inst->ParentChildBinding;

	// Search
	switch(*attr_name)
	{
	// Private Persistent
	case '%':
		// Read cached from tUDI_DriverModule
		// Write to permanent storage?
		break;
	// Private Volatile
	case '$':
		// Read from tUDI_DriverInstance
		break;
	// Sibling group
	case '^':
		// Read from parent's tUDI_DriverInstance
		break;
	// Parent-Visible
	case '@':
		// Read from tUDI_ChildBinding
		break;
	// Enumeration
	default:
		// Check associated tUDI_ChildBinding
		if( !inst->ParentChildBinding ) {
			return UDI_ATTR_NONE;
		}
		
		for( int i = 0; i < bind->nAttribs; i ++ )
		{
			const udi_instance_attr_list_t *at = &bind->Attribs[i];
			if( strcmp(at->attr_name, attr_name) == 0 )
			{
				if( actual_length )
					*actual_length = at->attr_length;
				udi_size_t len = (at->attr_length < attr_length) ? at->attr_length : attr_length;
				memcpy(attr_value, at->attr_value, len);
				return at->attr_type;
			}
		}
		break;
	}
	// - Priv
	// - enumeration attributes
	// - set attributes
	return UDI_ATTR_NONE;
}
