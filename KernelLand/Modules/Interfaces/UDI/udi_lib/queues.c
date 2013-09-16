/**
 * \file queues.c
 * \author John Hodge (thePowersGang)
 *
 * \brief UDI Queue Primatives
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORT(udi_enqueue);
EXPORT(udi_dequeue);

// === CODE ===
void udi_enqueue(udi_queue_t *new_el, udi_queue_t *old_el)
{
	new_el->next = old_el->next;
	new_el->prev = old_el;
	old_el->next = new_el;
	old_el->next->prev = new_el;
}
udi_queue_t *udi_dequeue(udi_queue_t *element)
{
	element->next->prev = element->prev;
	element->prev->next = element->next;
	return element;
}
