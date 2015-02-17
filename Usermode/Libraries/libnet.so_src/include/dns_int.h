/*
 */
#ifndef _DNS_INT_H_
#define _DNS_INT_H_

#include "dns.h"

extern size_t	DNS_int_EncodeQuery(void *buf, size_t bufsize, const char *name, enum eTypes type, enum eClass class);
extern int	DNS_int_ParseResponse(const void* packet, size_t return_len, void *info, handle_record_t* handle_record_t);

extern size_t	DNS_EncodeName(void *buf, const char *dotted_name);
extern int DNS_DecodeName(char dotted_name[256], const void *buf, size_t ofs, size_t space);
#endif

