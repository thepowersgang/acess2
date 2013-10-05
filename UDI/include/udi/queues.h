/**
 * \file udi_queues.h
 * \brief Queue Management Utility Functions
 */
#ifndef _UDI_QUEUES_H_
#define _UDI_QUEUES_H_

typedef struct udi_queue	udi_queue_t;

struct udi_queue
{
	struct udi_queue *next;
	struct udi_queue *prev;
};

extern void udi_enqueue(udi_queue_t *new_el, udi_queue_t *old_el);
extern udi_queue_t *udi_dequeue(udi_queue_t *element);

#define UDI_QUEUE_INIT(listhead)	((listhead)->next = (listhead)->prev = (listhead))
#define UDI_QUEUE_EMPTY(listhead)	((listhead)->next == (listhead)->prev)
// TODO: other queue macros

#endif

