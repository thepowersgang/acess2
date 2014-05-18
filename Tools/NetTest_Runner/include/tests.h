/*
 * tests.h
 * - List of tests used by main.c
 */
#ifndef _TESTS_H_
#define _TESTS_H_

#include "common.h"
#include <stdbool.h>

extern bool	Test_ARP_Basic(void);
extern bool	Test_TCP_Basic(void);
extern bool	Test_TCP_Reset(void);
extern bool	Test_TCP_WindowSizes(void);

#endif

