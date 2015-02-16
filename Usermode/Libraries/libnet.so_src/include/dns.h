/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * dns.h
 * - DNS Protocol Interface
 */
#ifndef _DNS_H_
#define _DNS_H_

#include <stddef.h>

enum eTypes
{
	TYPE_A = 1,
	TYPE_NS = 2,
	TYPE_CNAME = 5,
	TYPE_SOA = 6,
	TYPE_NULL = 10,
	TYPE_PTR = 12,
	TYPE_HINFO = 13,
	TYPE_MX = 15,
	TYPE_TXT = 16,
	QTYPE_STAR = 255,
};

enum eClass
{
	CLASS_IN = 1,
	CLASS_CH = 3,	// "Chaos"
	QCLASS_STAR = 255,
};

/**
 * \brief Handler for a DNS record obtained by DNS_Query
 * \param info	Value passed as the last argument to DNS_Query
 * \param name	NUL-terminated name associated with the returned record
 * \param type	Record type (may not be equal to requested)
 * \param class	Record class (may not be equal to requested)
 * \param rdlength	Length of data pointed to by 'rdata'
 * \param rdata	Record data
 */
typedef void	handle_record_t(void *info, const char *name, enum eTypes type, enum eClass class, unsigned int ttl, size_t rdlength, const void *rdata);

int DNS_Query(int ServerAType, const void *ServerAddr, const char *name, enum eTypes type, enum eClass class, handle_record_t* handle_record, void *info);

#endif

