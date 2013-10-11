/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * inifile.h
 * - .ini file parsing
 */
#ifndef _INIFILE_H_
#define _INIFILE_H_

typedef struct sInifile	tIniFile;

extern tIniFile	*IniFile_Load(const char *Path);
extern const char	*IniFile_Get(tIniFile *File, const char *Sect, const char *Key, const char *Default);
extern void	IniFile_Free(tIniFile *File);

#endif

