#ifndef HELPERS_H

#define HELPERS_H

#include <edi.h>

// Locally Defined
bool edi_string_equal(edi_string_t x,edi_string_t y);
bool descends_from(data_pointer object_class,edi_string_t desired_class);
data_pointer get_actual_class(edi_string_t ancestor,int32_t num_objects,edi_object_metadata_t *objects);

// Local Copy/set
void *memcpyd(void *dest, void *src, unsigned int count);

// Implementation Defined Common functions
void *memcpy(void *dest, void *src, unsigned int count);
void *memmove(void *dest, void *src, unsigned int count);
void *realloc(void *ptr, unsigned int size);

#endif
