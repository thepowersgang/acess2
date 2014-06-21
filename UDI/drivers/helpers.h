/*
 * UDI Driver helper macros
 */
#ifndef _UDI_HELPERS_H_
#define _UDI_HELPERS_H_

#if DEBUG_ENABLED
# define DEBUG_OUT(fmt, v...)	udi_debug_printf("%s: "fmt"\n", __func__ ,## v) 
#else
# define DEBUG_OUT(...)	do{}while(0)
#endif

#define ARRAY_COUNT(arr)	(sizeof(arr)/sizeof(arr[0]))

#define __EXPJOIN(a,b)	a##b
#define _EXPJOIN(a,b)	__EXPJOIN(a,b)
#define _EXPLODE(params...)	params
#define _ADDGCB(params...)	(udi_cb_t *gcb, params)
#define CONTIN(suffix, call, args, params)	extern void _EXPJOIN(suffix##$,__LINE__) _ADDGCB params;\
	call( _EXPJOIN(suffix##$,__LINE__), gcb, _EXPLODE args); } \
	void _EXPJOIN(suffix##$,__LINE__) _ADDGCB params { \
	rdata_t	*rdata = gcb->context; \
	(void)rdata;

/* Copied from http://projectudi.cvs.sourceforge.net/viewvc/projectudi/udiref/driver/udi_dpt/udi_dpt.h */
#define UDIH_SET_ATTR_BOOLEAN(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_BOOLEAN; \
		(attr)->attr_length = sizeof(udi_boolean_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define UDIH_SET_ATTR32(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_UBIT32; \
		(attr)->attr_length = sizeof(udi_ubit32_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define UDIH_SET_ATTR_ARRAY8(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_ARRAY8; \
		(attr)->attr_length = (len); \
		udi_memcpy((attr)->attr_value, (val), (len))

#define UDIH_SET_ATTR_STRING(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = (len); \
		udi_strncpy_rtrim((char *)(attr)->attr_value, (val), (len))
#define UDIH_SET_ATTR_STRFMT(attr, name, maxlen, fmt, v...) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = udi_snprintf((char *)(attr)->attr_value, (maxlen), (fmt) ,## v )

/**
 * \brief UDI PIO Helpers
 */
struct s_pio_ops {
	udi_pio_trans_t	*trans_list;
	udi_ubit16_t	list_length;
	udi_ubit16_t	pio_attributes;
	udi_ubit32_t	regset_idx;
	udi_ubit32_t	base_offset;
	udi_ubit32_t	length;
};
#define UDIH_PIO_OPS_ENTRY(list, attr, regset, base, len)	{list, ARRAY_COUNT(list), attr, regset, base, len}
#endif
