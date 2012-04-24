/*
 */
#ifndef _VIAVIDEO__COMMON_H_
#define _VIAVIDEO__COMMON_H_

typedef struct sVIAVideo_Dev	tVIAVideo_Dev;

struct sVIAVideo_Dev
{
	tPAddr	FramebufferPhys;
	tPAddr	MMIOPhys;
	
	void	*Framebuffer;
	Uint8	*MMIO;

	tDrvUtil_Video_BufInfo	BufInfo;

	size_t	FBSize;
};

#endif

