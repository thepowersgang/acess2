/*
 * AcessOS Microkernel Version
 * proc.h
 */
#ifndef _PROC_H
#define _PROC_H

// === CONSTANTS ===
#define GETMSG_IGNORE	((void*)-1)

// === TYPES ===
typedef struct sMessage {
	struct sMessage	*Next;
	Uint	Source;
	Uint	Length;
	Uint8	Data[];
} tMsg;	// sizeof = 12+

typedef struct sThread {
	struct sThread	*Next;
	 int	IsLocked;
	 int	Status;	//!< Thread Status
	
	Uint	TID;	//!< Thread ID
	Uint	TGID;	//!< Thread Group (Process)
	Uint	UID, GID;	//!< User and Group
	char	*ThreadName;	//!< Name of thread
	
	Uint	ESP, EBP, EIP;	//!< State on switch
	#if USE_PAE
	Uint64	PML4[3];	//!< Address Space
	#else
	Uint	CR3;	//!< Memory Space
	#endif
	
	Uint	KernelStack;	//!< Thread's Kernel Stack
	
	tMsg	*Messages;	//!< Message Queue
	tMsg	*LastMessage;	//!< Last Message (speeds up insertion)
	
	 int	Quantum, Remaining;	//!< Quantum Size and remaining timesteps
	 int	NumTickets;	//!< Priority - Chance of gaining CPU
	
	Uint	Config[NUM_CFG_ENTRIES];	//!< Per-process configuration
} tThread;	// sizeof = 68

enum {
	THREAD_STAT_NULL,
	THREAD_STAT_ACTIVE,
	THREAD_STAT_SLEEPING,
	THREAD_STAT_WAITING,
	THREAD_STAT_DEAD
};

typedef struct sTSS {
	Uint32	Link;
	Uint32	ESP0, SS0;
	Uint32	ESP1, SS1;
	Uint32	ESP2, SS2;
	Uint32	CR3;
	Uint32	EIP;
	Uint32	EFLAGS;
	Uint32	EAX, ECX, EDX, EBX;
	Uint32	ESP, EBP, ESI, EDI;
	Uint32	ES, CS, DS, SS, FS, GS;
	Uint32	LDTR;
	Uint16	Resvd, IOPB;	// IO Permissions Bitmap
} tTSS;

// === GLOBALS ===
extern tThread	*gCurrentThread;

// === FUNCTIONS ===
extern void	Proc_Start();
extern int	Proc_Clone(Uint *Err, Uint Flags);
extern void Proc_Exit();
extern void Proc_Yield();
extern void Proc_Sleep();
extern void Proc_SetTickets(int Num);
extern tThread	*Proc_GetThread(Uint TID);
extern void	Thread_Wake(tThread *Thread);

#endif
