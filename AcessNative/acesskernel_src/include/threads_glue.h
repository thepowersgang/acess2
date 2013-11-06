/*
 */
#ifndef _ACESSNATIVE__THREADS_GLUE_H_
#define _ACESSNATIVE__THREADS_GLUE_H_


extern void	Threads_Glue_Yield(void);
extern void	Threads_Glue_AcquireMutex(void **Lock);
extern void	Threads_Glue_ReleaseMutex(void **Lock);
extern void	Threads_Glue_SemInit(void **Ptr, int Val);
extern int	Threads_Glue_SemWait(void *Ptr, int Max);
extern int	Threads_Glue_SemSignal( void *Ptr, int AmmountToAdd );
extern void	Threads_Glue_SemDestroy( void *Ptr );

#endif

