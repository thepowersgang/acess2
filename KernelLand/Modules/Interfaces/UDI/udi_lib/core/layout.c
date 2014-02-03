/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_lib/core/layout.c
 * - udi_layout_t related functions
 */
#define DEBUG	1
#include <udi.h>
#include <acess.h>
#include <udi_internal.h>

// === CODE ===
#define alignto(ofs,type) \
	ofs + ((sizeof(type)-ofs%sizeof(type)) % sizeof(type));
#define _PUT(type,vtype) do{\
	ofs = alignto(ofs,type); \
	if(buf){\
		*(type*)(buf+ofs) = va_arg(*values,vtype);\
		LOG(#type" %p %x", buf+ofs, *(type*)(buf+ofs));\
	}\
	else if(values) \
		va_arg(*values,vtype); \
	ofs += sizeof(type); \
}while(0)

size_t _udi_marshal_step(void *buf, size_t cur_ofs, const udi_layout_t **layoutp, va_list *values)
{
	size_t	ofs = cur_ofs;
	const udi_layout_t	*layout = *layoutp;
	switch(*layout++)
	{
	case UDI_DL_UBIT8_T:	_PUT(udi_ubit8_t, UDI_VA_UBIT8_T);	break;
	case UDI_DL_SBIT8_T:	_PUT(udi_sbit8_t, UDI_VA_SBIT8_T);	break;
	case UDI_DL_UBIT16_T:	_PUT(udi_ubit16_t, UDI_VA_UBIT16_T);	break;
	case UDI_DL_SBIT16_T:	_PUT(udi_sbit16_t, UDI_VA_SBIT16_T);	break;
	case UDI_DL_UBIT32_T:	_PUT(udi_ubit32_t, UDI_VA_UBIT32_T);	break;
	case UDI_DL_SBIT32_T:	_PUT(udi_sbit32_t, UDI_VA_SBIT16_T);	break;
	case UDI_DL_BOOLEAN_T:	_PUT(udi_boolean_t, UDI_VA_BOOLEAN_T);	break;
	case UDI_DL_STATUS_T:	_PUT(udi_status_t, UDI_VA_STATUS_T);	break;
	
	case UDI_DL_INDEX_T:	_PUT(udi_index_t, UDI_VA_INDEX_T);	break;
	
	case UDI_DL_CHANNEL_T:	_PUT(udi_channel_t, UDI_VA_CHANNEL_T);	break;
	case UDI_DL_ORIGIN_T:	_PUT(udi_origin_t, UDI_VA_ORIGIN_T);	break;

	case UDI_DL_BUF:
		_PUT(udi_buf_t*,UDI_VA_POINTER);
		layout += 3;
		break;
	case UDI_DL_CB:
		_PUT(udi_cb_t*,UDI_VA_POINTER);
		break;
	case UDI_DL_INLINE_UNTYPED:
		_PUT(void*,UDI_VA_POINTER);
		break;
	case UDI_DL_INLINE_DRIVER_TYPED:
		_PUT(void*,UDI_VA_POINTER);
		break;
	case UDI_DL_MOVABLE_UNTYPED:
		_PUT(void*,UDI_VA_POINTER);
		break;
	
	case UDI_DL_INLINE_TYPED:
	case UDI_DL_MOVABLE_TYPED:
		_PUT(void*,UDI_VA_POINTER);
		while(*layout++ != UDI_DL_END)
			;
		break;

	default:
		Log_Error("UDI", "_udi_marshal_step - Unknown layout code %i", layout[-1]);
		return 0;
	}
	*layoutp = layout;
	return ofs;
}

size_t _udi_marshal_values(void *buf, const udi_layout_t *layout, va_list values)
{
	size_t	ofs = 0;
	while( *layout != UDI_DL_END )
	{
		ofs = _udi_marshal_step(buf, ofs, &layout, &values);
		if( ofs == 0 )
			return 0;
	}
	return ofs;
}
