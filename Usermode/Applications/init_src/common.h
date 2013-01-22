/*
 * Acess2 Userland init(8)
 * - Userland root process
 * 
 * common.h
 * - Common type definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

typedef struct sInitProgram	tInitProgram;

struct sKTerm
{
	 int	ID;
};

struct sSTerm
{
	uint32_t	FormatBits;
	unsigned int	BaudRate;
	char	Path[];
};

struct sDaemon
{
	char	*StdoutPath;	// heap
	char	*StderrPath;	// heap
};

union uProgTypes
{
	struct sKTerm	KTerm;
	struct sSTerm	STerm;
	struct sDaemon	Daemon;
};

enum eProgType
{
	PT_KTERM,
	PT_STERM,
	PT_DAEMON
};

struct sInitProgram
{
	tInitProgram	*Next;
	enum eProgType	Type;
	 int	CurrentPID;
	char	**Command;
	union uProgTypes	TypeInfo;
};

#endif

