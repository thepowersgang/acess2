/*
 * Acess2 GUI (AxWin3)
 */
#ifndef _MESSAGES_H_
#define _MESSAGES_H_

struct sAxWin_Message
{
	uint16_t	ID;
};

typedef struct sAxWin_Message	tAxWin_Message;

enum eMessages
{
	MSG_NULL,
	MSG_CREATEWIN
};

#endif

