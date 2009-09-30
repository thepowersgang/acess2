/*
 * Acess 2 Login
 */
#include <stdlib.h>
#include <stdio.h>
#include "header.h"

// === CONSTANTS ===

// === PROTOTYPES ===
 int	ValidateUser(char *Username, char *Password);
tUserInfo	*GetUserInfo(int UID);

// === GLOBALS ===
tUserInfo	gUserInfo;

// === CODE ===
/**
 * \fn int ValidateUser(char *Username, char *Password)
 * \brief Validates a users credentials
 * \return UID on success, -1 on failure
 */
int ValidateUser(char *Username, char *Password)
{
	if(Username == NULL)	return -1;
	if(Password == NULL)	return -1;
	if(strcmp(Username, "root") == 0)	return 0;
	return -1;
}

/**
 * \fn void GetUserInfo(int UID)
 * \brief Gets a users information
 */
tUserInfo *GetUserInfo(int UID)
{
	if(UID != 0)	return NULL;
	
	gUserInfo.UID = 0;
	gUserInfo.GID = 0;
	gUserInfo.Shell = "/Acess/Bin/CLIShell";
	gUserInfo.Home = "/Acess/Root";
	return &gUserInfo;
}

