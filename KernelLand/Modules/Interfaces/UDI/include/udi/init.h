/**
 * \file udi_init.h
 */
#ifndef _UDI_INIT_H_
#define _UDI_INIT_H_

typedef struct udi_init_s		udi_init_t;
typedef struct udi_primary_init_s	udi_primary_init_t;
typedef struct udi_secondary_init_s	udi_secondary_init_t;
typedef struct udi_ops_init_s	udi_ops_init_t;
typedef struct udi_cb_init_s	udi_cb_init_t;
typedef struct udi_cb_select_s	udi_cb_select_t;
typedef struct udi_gcb_init_s	udi_gcb_init_t;

typedef struct udi_init_context_s	udi_init_context_t;
typedef struct udi_limits_s		udi_limits_t;
typedef struct udi_chan_context_s	udi_chan_context_t;
typedef struct udi_child_chan_context_s	udi_child_chan_context_t;

typedef void	udi_op_t(void);
typedef udi_op_t * const	udi_ops_vector_t;

/**
 * \brief UDI Initialisation Structure
 * 
 * Defines how to initialise and use a UDI driver
 */
struct udi_init_s
{
	/**
	 * \brief Defines the primary region
	 * \note For secondary modules this must be NULL
	 */
	udi_primary_init_t	*primary_init_info;
	
	/**
	 * \brief Defines all secondary regions
	 * Pointer to a list (so, essentially an array) of ::udi_secondary_init_t
	 * It is terminated by an entry with ::udi_secondary_init_t.region_idx
	 * set to zero.
	 * \note If NULL, it is to be treated as an empty list
	 */
	udi_secondary_init_t	*secondary_init_list;
	
	/**
	 * \brief Channel operations
	 * Pointer to a ::udi_ops_init_t.ops_idx == 0  terminated list that
	 * defines the channel opterations usage for each ops vector implemented
	 * in this module.
	 * \note Must contain at least one entry for each metalanguage used
	 */
	udi_ops_init_t	*ops_init_list;
	
	/**
	 * \brief Control Blocks
	 */
	udi_cb_init_t	*cb_init_list;
	
	/**
	 * \brief Generic Control Blocks
	 */
	udi_gcb_init_t	*gcb_init_list;
	
	/**
	 * \brief Overrides for control blocks
	 * Allows a control block to override the ammount of scratch space it
	 * gets for a specific ops vector.
	 */
	udi_cb_select_t	*cb_select_list;
};


/**
 * \name Flags for ::udi_primary_init_t.mgmt_op_flags
 * \{
 */

/**
 * \brief Tells the environment that this operation may take some time
 * Used as a hint in scheduling tasks
 */
#define UDI_OP_LONG_EXEC	0x01

/**
 * \}
 */

/**
 * \brief Describes the Primary Region
 * Tells the environment how to set up the driver's primary region.
 */
struct udi_primary_init_s
{
	/**
	 * \brief Management Ops Vector
	 * Pointer to a list of functions for the Management Metalanguage
	 */
	udi_mgmt_ops_t	*mgmt_ops;
	
	/**
	 * \brief Flags for \a mgmt_ops
	 * Each entry in \a mgmt_ops is acommanied by an entry in this array.
	 * Each entry contains the flags that apply to the specified ops vector.
	 * \see UDI_OP_LONG_EXEC
	 */
	const udi_ubit8_t	*mgmt_op_flags;
	
	/**
	 * \brief Scratch space size
	 * Specifies the number of bytes to allocate for each control block
	 * passed by the environment.
	 * \note must not exceed ::UDI_MAX_SCRATCH
	 */
	udi_size_t	mgmt_scratch_requirement;
	
	/**
	 * \todo What is this?
	 */
	udi_ubit8_t	enumeration_attr_list_length;
	
	/**
	 * \brief Size in bytes to allocate to each instance of the primary
	 *        region
	 * Essentially the size of the driver's instance state
	 * \note Must be at least sizeof(udi_init_context_t) and not more
	 *       than UDI_MIN_ALLOC_LIMIT
	 */
	udi_size_t	rdata_size;
	
	/**
	 * \brief Size in bytes to allocate for each call to ::udi_enumerate_req
	 * \note Must not exceed UDI_MIN_ALLOC_LIMIT
	 */
	udi_size_t	child_data_size;
	
	/**
	 * \brief Number of path handles for each parent bound to this driver
	 * \todo What the hell are path handles?
	 */
	udi_ubit8_t	per_parent_paths;
};

/**
 * \brief Tells the environment how to create a secondary region
 */
struct udi_secondary_init_s
{
	/**
	 * \brief Region Index
	 * Non-zero driver-dependent index value that identifies the region
	 * \note This corresponds to a "region" declaration in the udiprops.txt
	 *       file.
	 */
	udi_index_t	region_idx;
	/**
	 * \brief Number of bytes to allocate
	 * 
	 * \note Again, must be between sizeof(udi_init_context_t) and
	 *       UDI_MIN_ALLOC_LIMIT
	 */
	udi_size_t	rdata_size;
};

/**
 * \brief Defines channel endpoints (ways of communicating with the driver)
 * 
 */
struct udi_ops_init_s
{
	/**
	 * \brief ops index number
	 * Used to uniquely this entry
	 * \note If this is zero, it marks the end of the list
	 */
	udi_index_t	ops_idx;
	/**
	 * \brief Metalanguage Index
	 * Defines what metalanguage is used
	 */
	udi_index_t	meta_idx;
	/**
	 * \brief Metalanguage Operation
	 * Defines what metalanguage operation is used
	 */
	udi_index_t	meta_ops_num;
	/**
	 * \brief Size of the context area
	 * \note If non-zero, must be at least 
	 */
	udi_size_t	chan_context_size;
	/**
	 * \brief Pointer to the operations
	 * Pointer to a <<meta>>_<<role>>_ops_t structure
	 */
	udi_ops_vector_t	*ops_vector;
	/**
	 * \brief Flags for each entry in \a ops_vector
	 */
	//const udi_ubit8_t	*op_flags;
};

/**
 * \brief Defines control blocks
 * Much the same as ::udi_ops_init_t
 */
struct udi_cb_init_s
{
	udi_index_t	cb_idx;
	udi_index_t	meta_idx;
	udi_index_t	meta_cb_num;
	udi_size_t	scratch_requirement;
	/**
	 * \brief Size of inline memory
	 */
	udi_size_t	inline_size;
	/**
	 * \brief Layout of inline memory
	 */
	udi_layout_t	*inline_layout;
};

/**
 * \brief Overrides the scratch size for an operation
 */
struct udi_cb_select_s
{
	udi_index_t	ops_idx;
	udi_index_t	cb_idx;
};

/**
 * \brief General Control Blocks
 * These control blocks can only be used as general data storage, not
 * for any channel operations.
 */
struct udi_gcb_init_s
{
	udi_index_t	cb_idx;
	udi_size_t	scratch_requirement;
};


// ===
// ===
/**
 * \brief Environement Imposed Limits
 */
struct udi_limits_s
{
	/**
	 * \brief Maximum legal ammount of memory that can be allocated
	 */
	udi_size_t	max_legal_alloc;
	
	/**
	 * \brief Maximum ammount of guaranteed memory
	 */
	udi_size_t	max_safe_alloc;
	/**
	 * \brief Maximum size of the final string from ::udi_trace_write
	 *        or ::udi_log_write
	 */
	udi_size_t	max_trace_log_formatted_len;
	/**
	 * \brief Maximum legal size of an instanct attribute value
	 */
	udi_size_t	max_instance_attr_len;
	/**
	 * \brief Minumum time difference (in nanoseconds between unique values
	 *        returned by ::udi_time_current
	 */
	udi_ubit32_t	min_curtime_res;
	/**
	 * \brief Minimum resolution of timers
	 * \see ::udi_timer_start_repeating, ::udi_timer_start
	 */
	udi_ubit32_t	min_timer_res;
} PACKED;

/**
 * \brief Primary Region Context data
 */
struct udi_init_context_s
{
	udi_index_t	region_idx;
	udi_limits_t	limits;
};

/**
 * \brief Channel context data
 */
struct udi_chan_context_s
{
	/**
	 * \brief Pointer to the driver instance's initial region data
	 */
	void	*rdata;
} PACKED;

/**
 * \brief Child Channel context
 */
struct udi_child_chan_context_s
{
	/**
	 * \brief Pointer to the driver instance's initial region data
	 */
	void	*rdata;
	/**
	 * \brief Some sort of unique ID number
	 */
	udi_ubit32_t	child_ID;
};

#endif
