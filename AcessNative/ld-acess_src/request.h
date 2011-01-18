/*
 * Acess2 - AcessNative
 * ld-acess
 *
 * request.h - IPC Request common header
 */

#ifndef _REQUEST_H_
#define _REQUEST_H_

typedef struct {
	char	Type;
	 int	Length;
	char	Data[];
}	tOutValue;

typedef struct {
	char	Type;
	 int	Length;
	void	*Data;
}	tInValue;

extern int SendRequest(int RequestID, int NumOutput, tOutValue **Output,
	int NumInput, tInValue **Input);

#endif
