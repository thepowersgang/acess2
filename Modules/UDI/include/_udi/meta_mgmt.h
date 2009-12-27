/*
 * Acess2 UDI Support
 * _udi/meta_mgmt.h
 * - Mangement Metalanguage
 */
#ifndef _UDI_META_MGMT_H_
#define _UDI_META_MGMT_H_

typedef struct {
	udi_cb_t	gcb;
} udi_mgmt_cb_t;


typedef struct {
	udi_cb_t	gcb;
	udi_trevent_t	trace_mask;
	udi_index_t	meta_idx;
} udi_usage_cb_t;


typedef struct {
	udi_cb_t	gcb;
	udi_ubit32_t	child_ID;
	void	*child_data;
	udi_instance_attr_list_t	*attr_list;
	udi_ubit8_t	attr_valid_length;
	const udi_filter_element_t	*filter_list;
	udi_ubit8_t	filter_list_length;
	udi_ubit8_t	parent_ID;
} udi_enumerate_cb_t;


typedef void udi_usage_ind_op_t(udi_usage_cb_t *, udi_ubit8_t);
typedef void udi_enumerate_req_op_t(udi_enumerate_cb_t *, udi_ubit8_t);
typedef void udi_devmgmt_req_op_t(udi_mgmt_cb_t *, udi_ubit8_t);
typedef void udi_final_cleanup_req_op_t(udi_mgmt_cb_t *);

typedef const struct {
	udi_usage_ind_op_t	*usage_ind_op;
	udi_enumerate_req_op_t	*enumerate_req_op;
	udi_devmgmt_req_op_t	*devmgmt_req_op;
	udi_final_cleanup_req_op_t	*final_cleanup_req_op;
} udi_mgmt_ops_t;

#endif
