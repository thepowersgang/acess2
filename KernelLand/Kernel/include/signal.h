/*
 * Acess2 Kernel
 * Signal List
 */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#define SIG_DFL	NULL

enum eSignals {
	SIGNONE =  0,	// No signal
	SIGHUP  =  1,
	SIGINT  =  2, 	// C
	SIGQUIT =  3,	// POSIX
	SIGILL  =  4,	// C
	SIGTRAP =  5,	// POSIX
	SIGABRT =  6,	// C?
	SIGBUS  =  7, 	// ?
	SIGFPE  =  8, 	// C
	SIGKILL =  9,	// POSIX
	SIGUSR1 = 10,
	SIGSEGV = 11,	// C
	SIGUSR2 = 12,
	SIGPIPE = 13,	// 
	SIGALRM = 14,
	SIGTERM = 15,	
	_SIG16  = 16,
	SIGCHLD = 17,
	SIGCONT = 18,
	SIGSTOP = 19,
	NSIGNALS
};

#endif
