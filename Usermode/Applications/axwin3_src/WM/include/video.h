/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/video.h
 * - Video code header
 */
#ifndef _VIDEO_H_
#define _VIDEO_H_

extern void	Video_Update(void);
extern void	Video_Blit(uint32_t *Source, short DstX, short DstY, short W, short H);
extern void	Video_FillRect(int X, int Y, int W, int H, uint32_t Colour);

#endif

