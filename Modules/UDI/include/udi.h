/*
 * Acess2 UDI Support
 * udi.h
 */
#ifndef _UDI_H_
#define _UDI_H_

#include <udi_arch.h>

typedef void	udi_op_t(void);

typedef udi_op_t const	*udi_ops_vector_t;

typedef const udi_ubit8_t	udi_layout_t;

typedef udi_ubit32_t	udi_trevent_t;

#include <_udi/values.h>

typedef struct {
     udi_channel_t	channel;
     void	*context;
     void	*scratch;
     void	*initiator_context;
     udi_origin_t	origin;
} udi_cb_t;

/**
 * \name Attributes
 * \{
 */
#define UDI_MAX_ATTR_NAMELEN 32
#define UDI_MAX_ATTR_SIZE    64
typedef struct {
     char	attr_name[UDI_MAX_ATTR_NAMELEN];
     udi_ubit8_t	attr_value[UDI_MAX_ATTR_SIZE];
     udi_ubit8_t	attr_length;
     udi_instance_attr_type_t	attr_type;
} udi_instance_attr_list_t;

typedef struct {
     char	attr_name[UDI_MAX_ATTR_NAMELEN];
     udi_ubit8_t	attr_min[UDI_MAX_ATTR_SIZE];
     udi_ubit8_t	attr_min_len;
     udi_ubit8_t	attr_max[UDI_MAX_ATTR_SIZE];
     udi_ubit8_t	attr_max_len;
     udi_instance_attr_type_t	attr_type;
     udi_ubit32_t	attr_stride;
} udi_filter_element_t;
/**
 * \}
 */



#include <_udi/meta_mgmt.h>

/**
 */
typedef const struct {
	udi_index_t	cb_idx;
	udi_size_t	scratch_requirement;
} udi_gcb_init_t;

/**
 */
typedef const struct {
	udi_index_t	ops_idx;
	udi_index_t	cb_idx;
} udi_cb_select_t;

/**
 */
typedef const struct {
	udi_index_t	cb_idx;
	udi_index_t	meta_idx;
	udi_index_t	meta_cb_num;
	udi_size_t	scratch_requirement;
	udi_size_t	inline_size;
	udi_layout_t	*inline_layout;
} udi_cb_init_t;

/**
 * The \a udi_ops_init_t structure contains information the environment
 * needs to subsequently create channel endpoints for a particular type of ops
 * vector and control block usage. This structure is part of \a udi_init_info.
 */
typedef const struct {
	udi_index_t	ops_idx;	//!< Non Zero driver assigned number
	udi_index_t	meta_idx;	//!< Metalanguage Selector
	udi_index_t	meta_ops_num;	//!< Metalanguage Operation
	udi_size_t	chan_context_size;
	udi_ops_vector_t	*ops_vector;	//!< Array of function pointers
	const udi_ubit8_t	*op_flags;
} udi_ops_init_t;

/**
 */
typedef const struct {
	udi_index_t	region_idx;
	udi_size_t	rdata_size;
} udi_secondary_init_t;


/**
 */
typedef const struct {
	udi_mgmt_ops_t	*mgmt_ops;
	const udi_ubit8_t	*mgmt_op_flags;
	udi_size_t	mgmt_scratch_requirement;
	udi_ubit8_t	enumeration_attr_list_length;
	udi_size_t	rdata_size;
	udi_size_t	child_data_size;
	udi_ubit8_t	per_parent_paths;
} udi_primary_init_t;

/**
 */
typedef const struct {
	udi_primary_init_t	*primary_init_info;
	udi_secondary_init_t	*secondary_init_list;	//!< Array
	udi_ops_init_t	*ops_init_list;
	udi_cb_init_t	*cb_init_list;
	udi_gcb_init_t	*gcb_init_list;
	udi_cb_select_t	*cb_select_list;
} udi_init_t;


//
// == Regions ==
//

/**
 * udi_limits_t reflects implementation-dependent system limits, such as
 * memory allocation and timer resolution limits, for a particular region. These
 * limits may vary from region to region, but will remain constant for the life of
 * a region.
 */
typedef struct {
	udi_size_t	max_legal_alloc;
	udi_size_t	max_safe_alloc;
	udi_size_t	max_trace_log_formatted_len;
	udi_size_t	max_instance_attr_len;
	udi_ubit32_t	min_curtime_res;
	udi_ubit32_t	min_timer_res;
} udi_limits_t;

/**
The \a udi_init_context_t structure is stored at the front of the region
data area of each newly created region, providing initial data that a driver will
need to begin executing in the region. A pointer to this structure (and therefore
the region data area as a whole) is made available to the driver as the initial
channel context for its first channel.
 */
typedef struct {
	udi_index_t	region_idx;
	udi_limits_t	limits;
} udi_init_context_t;

typedef struct {
	void	*rdata;
} udi_chan_context_t;

#endif
