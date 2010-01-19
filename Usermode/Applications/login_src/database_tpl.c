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
	printf("Username: %s\n", Username);
	if(Username == NULL)	return -1;
	if(Password == NULL)	return -1;
	if(strcmp(Username, "root") == 0)	return 0;
	if(strcmp(Username, "tpg") == 0)	return 1;
	return -1;
}

/**
 * \fn void GetUserInfo(int UID)
 * \brief Gets a users information
 */
tUserInfo *GetUserInfo(int UID)
{	
	gUserInfo.UID = UID;
	gUserInfo.GID = UID;
	gUserInfo.Shell = "/Acess/Bin/CLIShell";
	switch(UID)
	{
	case 0:
		gUserInfo.Home = "/Acess/Root";
		break;
	case 1:
		gUserInfo.Home = "/Acess/Users/tpg";
		break;
	default:
		gUserInfo.Home = "/Acess/Users/Guest";
		break;
	}
	return &gUserInfo;
}

