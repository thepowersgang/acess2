/* 
 * Acess2
 * - Abstract Data Types
 */
#ifndef _ADT_H_
#define _ADT_H_

/**
 * \name Ring Buffers
 * \{
 */
typedef struct sRingBuffer
{
	size_t	Start;	//!< Start of data in ring buffer
	size_t	Length;	//!< Number of data bytes in buffer
	size_t	Space;	//!< Allocated space in buffer
	tShortSpinlock	Lock;	//!< Lock to prevent collisions
	char	Data[];	//!< Buffer
}	tRingBuffer;

/**
 * \brief Create a ring buffer \a Space bytes large
 * \param Space	Ammount of space to allocate within the buffer
 * \return Pointer to the buffer structure
 */
extern tRingBuffer	*RingBuffer_Create(size_t Space);
/**
 * \brief Read at most \a Length bytes from the buffer
 * \param Dest	Destinaton buffer
 * \param Buffer	Source ring buffer
 * \param Length	Requested number of bytes
 * \return Number of bytes read
 */
extern size_t	RingBuffer_Read(void *Dest, tRingBuffer *Buffer, size_t Length);
/**
 * \brief Write at most \a Length bytes to the buffer
 * \param Buffer	Destination ring buffer
 * \param Source	Source buffer
 * \param Length	Provided number of bytes
 * \return Number of bytes written
 */
extern size_t	RingBuffer_Write(tRingBuffer *Buffer, const void *Source, size_t Length);
/**
 * \}
 */


#endif
