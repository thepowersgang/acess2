/*
 * AcessOS Basic C Library
 */
#include "stdio_int.h"

extern char **_envp;
extern struct sFILE	_iob[];
extern struct sFILE	*stdin;
extern struct sFILE	*stdout;
extern struct sFILE	*stderr;

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
	stdin = &_iob[0];
	stdin->FD = 0;	stdin->Flags = FILE_FLAG_MODE_READ;
	stdout = &_iob[1];
	stdout->FD = 1;	stdout->Flags = FILE_FLAG_MODE_WRITE;
	stderr = &_iob[2];
	stderr->FD = 2;	stderr->Flags = FILE_FLAG_MODE_WRITE;
	
	return 1;
}
