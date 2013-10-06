/**
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi/attr.h
 * - Instance Attribute Management
 */
#ifndef _UDI__ATTR_H_
#define _UDI__ATTR_H_

typedef struct udi_instance_attr_list_s	udi_instance_attr_list_t;
typedef udi_ubit8_t	udi_instance_attr_type_t;

/* Instance attribute limits */
#define UDI_MAX_ATTR_NAMELEN	32
#define UDI_MAX_ATTR_SIZE		64

#define UDI_ATTR32_SET(aval, v) \
	{ udi_ubit32_t vtmp = (v); \
	(aval)[0] = (vtmp) & 0xff; \
	(aval)[1] = ((vtmp) >> 8) & 0xff; \
	(aval)[2] = ((vtmp) >> 16) & 0xff; \
	(aval)[3] = ((vtmp) >> 24) & 0xff; }
#define UDI_ATTR32_GET(aval) \
	((aval)[0] + ((aval)[1] << 8) + \
	((aval)[2] << 16) + ((aval)[3] << 24))
#define UDI_ATTR32_INIT(v) \
	{ (v) & 0xff, ((v) >> 8) & 0xff, \
	((v) >> 16) & 0xff, ((v) >> 24) & 0xff }

/**
 * \brief Instance Attribute
 */
struct udi_instance_attr_list_s
{
     char	attr_name[UDI_MAX_ATTR_NAMELEN];
     udi_ubit8_t	attr_value[UDI_MAX_ATTR_SIZE];
     udi_ubit8_t	attr_length;
     udi_instance_attr_type_t	attr_type;
};


/**
 * \brief Instance Attribute Types
 * \see ::udi_instance_attr_type_t
 */
enum
{
	UDI_ATTR_NONE,
	UDI_ATTR_STRING,
	UDI_ATTR_ARRAY8,
	UDI_ATTR_UBIT32,
	UDI_ATTR_BOOLEAN,
	UDI_ATTR_FILE
};

typedef void udi_instance_attr_get_call_t(udi_cb_t *gcb, udi_instance_attr_type_t attr_type, udi_size_t actual_length);

extern void udi_instance_attr_get(udi_instance_attr_get_call_t *callback, udi_cb_t *gcb,
	const char *attr_name, udi_ubit32_t child_ID,
	void *attr_value, udi_size_t attr_length
	);

typedef void udi_instance_attr_set_call_t(udi_cb_t *gcb, udi_status_t status);

extern void udi_instance_attr_set(udi_instance_attr_set_call_t *callback, udi_cb_t *gcb,
	const char *attr_name, udi_ubit32_t child_ID,
	const void *attr_value, udi_size_t attr_length, udi_ubit8_t attr_type
	);

#define UDI_INSTANCE_ATTR_DELETE(callback, gcb, attr_name) \
	udi_instance_attr_set(callbac, gcb, attr_name, NULL, NULL, 0, UDI_ATTR_NONE)


#endif
