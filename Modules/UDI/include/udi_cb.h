/**
 * \file udi_cb.h
 */
#ifndef _UDI_CB_H_
#define _UDI_CB_H_

typedef struct udi_cb_s	udi_cb_t;

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


#endif
