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

#endif

