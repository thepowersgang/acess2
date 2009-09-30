/*
 * AcessOS Basic C Library
 */
#include "stdio_int.h"

extern char **_envp;
extern struct sFILE	_iob[];

/**
 * \fn int SoMain()
 * \brief Stub Entrypoint
 * \param BaseAddress	Unused - Load Address of libc
 * \param argc	Unused - Argument Count (0 for current version of ld-acess)
 * \param argv	Unused - Arguments (NULL for current version of ld-acess)
 * \param envp	Environment Pointer
 */
int SoMain(unsigned int BaseAddress, int argc, char **argv, char **envp)
{
	// Init for env.c
	_envp = envp;
	
	// Init FileIO Pointers
	_iob[0].FD = 0;	_iob[0].Flags = FILE_FLAG_MODE_READ;
	_iob[1].FD = 1;	_iob[1].Flags = FILE_FLAG_MODE_WRITE;
	_iob[2].FD = 2;	_iob[2].Flags = FILE_FLAG_MODE_WRITE;
	
	return 1;
}
