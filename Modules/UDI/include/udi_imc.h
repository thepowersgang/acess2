/**
 * \file udi_imc.h
 * \brief Inter-Module Communication
 */
#ifndef _UDI_IMC_H_
#define _UDI_IMC_H_

typedef void udi_channel_anchor_call_t(udi_cb_t *gcb, udi_channel_t anchored_channel);
typedef void udi_channel_spawn_call_t(udi_cb_t *gcb, udi_channel_t new_channel);

typedef struct udi_channel_event_cb_s	udi_channel_event_cb_t;

typedef void udi_channel_event_ind_op_t(udi_channel_event_cb_t *cb);

/**
 * \brief Anchors a channel end to the current region
 */
extern void udi_channel_anchor(
	udi_channel_anchor_call_t *callback, udi_cb_t *gcb,
	udi_channel_t channel, udi_index_t ops_idx, void *channel_context
	);

/**
 * \brief Created a new channel between two regions
 */
extern void udi_channel_spawn(
	udi_channel_spawn_call_t *callback,
	udi_cb_t *gcb,
	udi_channel_t channel,
	udi_index_t spawn_idx,
	udi_index_t ops_idx,
	void *channel_context
	);

/**
 * \brief Attaches a new context pointer to the current channel
 */
extern void udi_channel_set_context(
	udi_channel_t target_channel,
	void *channel_context
	);
/**
 * \brief 
 */
extern void udi_channel_op_abort(
	udi_channel_t target_channel,
	udi_cb_t *orig_cb
	);

/**
 * \brief Closes an open channel
 */
extern void udi_channel_close(udi_channel_t channel);

/**
 * \brief Describes a channel event
 */
struct udi_channel_event_cb_s
{
	udi_cb_t gcb;
	udi_ubit8_t event;
	union {
		struct {
			udi_cb_t *bind_cb;
		} internal_bound;
		struct {
			udi_cb_t *bind_cb;
			udi_ubit8_t parent_ID;
			udi_buf_path_t *path_handles;
		} parent_bound;
		udi_cb_t *orig_cb;
	}	params;
};
/* Channel event types */
#define UDI_CHANNEL_CLOSED                0
#define UDI_CHANNEL_BOUND                 1
#define UDI_CHANNEL_OP_ABORTED            2

/**
 * \brief Proxy function 
 */
extern void udi_channel_event_ind(udi_channel_event_cb_t *cb);

/**
 * \brief Called when channel event is completed
 */
extern void udi_channel_event_complete(
	udi_channel_event_cb_t *cb, udi_status_t status
	);


#endif
