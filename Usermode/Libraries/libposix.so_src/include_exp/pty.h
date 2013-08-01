/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * pty.h
 * - PTY Management
 *
 * BSD Conforming (Non-POSIX)
 */
#ifndef _LIBPOSIX__PTY_H_
#define _LIBPOSIX__PTY_H_

#include <termios.h>
#include <unistd.h>

extern int openpty(int *amaster, int *aslave, char *name, const struct termios *termp, const struct winsize *winp);
extern pid_t forkpty(int *amaster, char *name, const struct termios *termp, const struct winsize *winp);
// - utmp.h?
extern int login_tty(int fd);

#endif

