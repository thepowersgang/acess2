/**
 * \file udi_meta_mgmt.h
 */
#ifndef _UDI_META_MGMT_H_
#define _UDI_META_MGMT_H_

typedef struct udi_mgmt_ops_s	udi_mgmt_ops_t;
typedef struct udi_mgmt_cb_s	udi_mgmt_cb_t;
typedef struct udi_usage_cb_s	udi_usage_cb_t;
typedef struct udi_filter_element_s	udi_filter_element_t;
typedef struct udi_enumerate_cb_s	udi_enumerate_cb_t;

/**
 * \name Specify Usage
 * \{
 */
typedef void udi_usage_ind_op_t(udi_usage_cb_t *cb, udi_ubit8_t resource_level);
/* Values for resource_level */
#define UDI_RESOURCES_CRITICAL     1
#define UDI_RESOURCES_LOW          2
#define UDI_RESOURCES_NORMAL       3
#define UDI_RESOURCES_PLENTIFUL    4
/* Proxy */
extern udi_usage_ind_op_t	udi_static_usage;
/**
 * \}
 */

typedef void udi_usage_res_op_t(udi_usage_cb_t *cb);

/**
 * \name Enumerate this driver
 * \{
 */
typedef void udi_enumerate_req_op_t(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level);
/* Values for enumeration_level */
#define UDI_ENUMERATE_START           1
#define UDI_ENUMERATE_START_RESCAN    2
#define UDI_ENUMERATE_NEXT            3
#define UDI_ENUMERATE_NEW             4
#define UDI_ENUMERATE_DIRECTED        5
#define UDI_ENUMERATE_RELEASE         6
/* Proxy */
extern udi_enumerate_req_op_t	udi_enumerate_no_children;
/**
 * \}
 */

/**
 * \name Enumeration Acknowlagement
 * \{
 */
typedef void udi_enumerate_ack_op_t(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_result, udi_index_t ops_idx);
/* Values for enumeration_result */
#define UDI_ENUMERATE_OK             0
#define UDI_ENUMERATE_LEAF           1
#define UDI_ENUMERATE_DONE           2
#define UDI_ENUMERATE_RESCAN         3
#define UDI_ENUMERATE_REMOVED        4
#define UDI_ENUMERATE_REMOVED_SELF   5
#define UDI_ENUMERATE_RELEASED       6
#define UDI_ENUMERATE_FAILED         255
/**
 * \}
 */

/**
 * \name 
 * \{
 */
typedef void udi_devmgmt_req_op_t(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID);
/* Values for mgmt_op */
#define UDI_DMGMT_PREPARE_TO_SUSPEND 1
#define UDI_DMGMT_SUSPEND            2
#define UDI_DMGMT_SHUTDOWN           3
#define UDI_DMGMT_PARENT_SUSPENDED   4
#define UDI_DMGMT_RESUME             5
#define UDI_DMGMT_UNBIND             6

typedef void udi_devmgmt_ack_op_t(udi_mgmt_cb_t *cb, udi_ubit8_t flags, udi_status_t status);
/* Values for flags */
#define UDI_DMGMT_NONTRANSPARENT          (1U<<0)
/* Meta-Specific Status Codes */
#define UDI_DMGMT_STAT_ROUTING_CHANGE     (UDI_STAT_META_SPECIFIC|1)
/**
 * \}
 */
typedef void udi_final_cleanup_req_op_t(udi_mgmt_cb_t *cb);
typedef void udi_final_cleanup_ack_op_t(udi_mgmt_cb_t *cb);





struct udi_mgmt_ops_s
{
	udi_usage_ind_op_t	*usage_ind_op;
	udi_enumerate_req_op_t	*enumerate_req_op;
	udi_devmgmt_req_op_t	*devmgmt_req_op;
	udi_final_cleanup_req_op_t	*final_cleanup_req_op;
};

struct udi_mgmt_cb_s
{
	udi_cb_t	gcb;
};

struct udi_usage_cb_s
{
	udi_cb_t	gcb;
	udi_trevent_t	trace_mask;
	udi_index_t	meta_idx;
};


struct udi_filter_element_s
{
     char	attr_name[UDI_MAX_ATTR_NAMELEN];
     udi_ubit8_t	attr_min[UDI_MAX_ATTR_SIZE];
     udi_ubit8_t	attr_min_len;
     udi_ubit8_t	attr_max[UDI_MAX_ATTR_SIZE];
     udi_ubit8_t	attr_max_len;
     udi_instance_attr_type_t	attr_type;
     udi_ubit32_t	attr_stride;
};
struct udi_enumerate_cb_s
{
     udi_cb_t	gcb;
     udi_ubit32_t	child_ID;
     void	*child_data;
     udi_instance_attr_list_t	*attr_list;
     udi_ubit8_t	attr_valid_length;
     const udi_filter_element_t	*filter_list;
     udi_ubit8_t	filter_list_length;
     udi_ubit8_t	parent_ID;
};
/* Special parent_ID filter values */
#define UDI_ANY_PARENT_ID	0

#endif
