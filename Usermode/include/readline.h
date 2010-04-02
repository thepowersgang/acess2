/*
 * Acess2 Library Suite
 * - Readline
 * 
 * Text mode entry with history
 */
#ifndef _READLINE_H_
#define _READLINE_H_

// === TYPES ===
typedef struct sReadline	tReadline;

// === STRUCTURES ===
struct sReadline
{
	 int	UseHistory;	// Boolean
	
	 int	NumHistory;
	char	**History;
};

// === FUNCTIONS ===
/**
 * \brief Read a line from stdin
 */
extern char	*Readline(tReadline *Info);

#endif
