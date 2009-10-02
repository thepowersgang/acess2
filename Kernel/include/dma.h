/*
 * Acess2 DMA Driver
 */
#ifndef _DMA_H_
#define _DMA_H_

extern void	DMA_SetChannel(int channel, int length, int read);
extern int	DMA_ReadData(int channel, int count, void *buffer);

#endif
