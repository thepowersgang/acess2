/*
 * Acess2 GUIv4 Library
 * - By John Hodge (thePowersGang)
 *
 * common.h
 * - Library internal header
 */
#ifndef _LIBAXWIN4_COMMON_H_
#define _LIBAXWIN4_COMMON_H_

#include <serialisation.hpp>

namespace AxWin {

extern void	SendMessage(CSerialiser& message);
extern void	RecvMessage(CDeserialiser& message);
extern CDeserialiser	GetSyncReply(CSerialiser& request, unsigned int Message);

};

struct sAxWin4_Window
{
	unsigned int	m_id;
	 int	m_fd;
	void	*m_buffer;
};

#endif

