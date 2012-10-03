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

// === FUNCTIONS ===
/**
 * 
 */
extern tReadline	*Readline_Init(int UseHistory);

/**
 * \brief Read a line from stdin
 * \return Heap string containing the command string (to be free'd by the caller)
 */
extern char	*Readline(tReadline *Info);

/**
 * \brief Read a line from stdin (non-blocking)
 * \return Heap string containing the command string (to be free'd by the caller)
 */
extern char	*Readline_NonBlock(tReadline *Info);

#endif
