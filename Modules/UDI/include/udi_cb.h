/**
 * \file udi_cb.h
 */
#ifndef _UDI_CB_H_
#define _UDI_CB_H_

typedef struct udi_cb_s	udi_cb_t;
typedef void udi_cb_alloc_call_t(udi_cb_t *gcb, udi_cb_t *new_cb);
typedef void udi_cb_alloc_batch_call_t(udi_cb_t *gcb, udi_cb_t *first_new_cb);
typedef void udi_cancel_call_t(udi_cb_t *gcb);

#define UDI_GCB(mcb)	(&(mcb)->gcb)
#define UDI_MCB(gcb, cb_type)	((cb_type *)(gcb))

/**
 * \brief Describes a generic control block
 * \note Semi-opaque
 */
struct udi_cb_s
{
	/**
	 * \brief Channel associated with the control block
	 */
	udi_channel_t	channel;
	/**
	 * \brief Current state
	 * \note Driver changable
	 */
	void	*context;
	/**
	 * \brief CB's scratch area
	 */
	void	*scratch;
	/**
	 * \brief ???
	 */
	void	*initiator_context;
	/**
	 * \brief Request Handle?
	 */
	udi_origin_t	origin;
};

extern void udi_cb_alloc (
	udi_cb_alloc_call_t	*callback,
	udi_cb_t	*gcb,
	udi_index_t	cb_idx,
	udi_channel_t	default_channel
	);

extern void udi_cb_alloc_dynamic(
	udi_cb_alloc_call_t	*callback,
	udi_cb_t	*gcb,
	udi_index_t	cb_idx,
	udi_channel_t	default_channel,
	udi_size_t	inline_size,
	udi_layout_t	*inline_layout
	);

extern void udi_cb_alloc_batch(
	udi_cb_alloc_batch_call_t	*callback,
	udi_cb_t	*gcb,
	udi_index_t	cb_idx,
	udi_index_t	count,
	udi_boolean_t	with_buf,
	udi_size_t	buf_size,
	udi_buf_path_t	path_handle
	);

extern void udi_cb_free(udi_cb_t *cb);

extern void udi_cancel(udi_cancel_call_t *callback, udi_cb_t *gcb);




#endif
