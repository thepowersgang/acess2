/*
AcessOS Basic C Library
*/

extern char **_envp;

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
	
	return 1;
}
