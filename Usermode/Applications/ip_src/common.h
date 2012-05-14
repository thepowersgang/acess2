/*
 * Acess2 ip command
 * - By John Hodge (thePowersGang)
 * 
 * common.h
 * - Shared header
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <net.h>
#include <acess/sys.h>

#define FILENAME_MAX	255
#define IPSTACK_ROOT	"/Devices/ip"

#define ntohs(v)	(((v&0xFF)<<8)|((v>>8)&0xFF))

extern void	PrintUsage(const char *ProgName);
extern int	ParseIPAddress(const char *Address, uint8_t *Dest, int *SubnetBits);
extern int	Addr_main(int argc, char *argv[]);
extern int	Routes_main(int argc, char *argv[]);

#endif

