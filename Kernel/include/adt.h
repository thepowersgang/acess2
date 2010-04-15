/* 
 * Acess2
 * - Abstract Data Types
 */
#ifndef _ADT_H_
#define _ADT_H_

/**
 * \name Ring Buffer
 * \{
 */
typedef struct sRingBuffer
{
	size_t	Start;
	size_t	Length;
	size_t	Space;
	char	Data[];
}	tRingBuffer;

/**
 * \brief Create a ring buffer \a Space bytes large
 * \param Space	Ammount of space to allocate within the buffer
 * \return Pointer to the buffer structure
 */
extern tRingBuffer	*RingBuffer_Create(size_t Space);
extern size_t	RingBuffer_Read(void *Dest, tRingBuffer *Buffer, size_t Length);
extern size_t	RingBuffer_Write(tRingBuffer *Buffer, void *Source, size_t Length);
/**
 * \}
 */


#endif
