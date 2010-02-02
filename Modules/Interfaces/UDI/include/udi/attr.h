/**
 * \file udi_attr.h
 */
#ifndef _UDI_ATTR_H_
#define _UDI_ATTR_H_

typedef struct udi_instance_attr_list_s	udi_instance_attr_list_t;
typedef udi_ubit8_t	udi_instance_attr_type_t;

/* Instance attribute limits */
#define UDI_MAX_ATTR_NAMELEN	32
#define UDI_MAX_ATTR_SIZE		64

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


#endif
