/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * dirent.h
 * - Directory Reading
 */
#ifndef _LIBPOSIX__SYS__DIRENT_H_
#define _LIBPOSIX__SYS__DIRENT_H_

#define NAME_MAX	255

struct dirent
{
	ino_t	d_ino;
	char	d_name[NAME_MAX+1];
};

typedef struct DIR_s	DIR;

extern int	closedir(DIR *);
extern DIR	*opendir(const char *);
extern struct dirent	*readdir(DIR *);
extern int	readdir_r(DIR *, struct dirent *, struct dirent **);
extern void	rewinddir(DIR *);
extern void	seekdir(DIR *, long int);
extern long int	telldir(DIR *);

#endif

