/*
 * Acess2 Login Shell
 */
#ifndef _HEADER_H_
#define _HEADER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <acess/sys.h>

// === TYPES ===
typedef struct {
	 int	UID;
	 int	GID;
	char	*Home;
	char	*Shell;
} tUserInfo;

// === PROTOTYPES ===
// --- User Database ---
extern int	ValidateUser(char *Username, char *Password);
extern tUserInfo	*GetUserInfo(int UID);

#endif
