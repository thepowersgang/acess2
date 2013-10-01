/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * build.h
 * - Actual build steps (exposed functions)
 */
#ifndef _BUILD_H_
#define _BUILD_H_

#include "inifile.h"
#include "udiprops.h"

extern int	Build_CompileFile(tIniFile *opts, const char *abi, tUdiprops *udiprops, tUdiprops_Srcfile *srcfile);
extern int	Build_LinkObjects(tIniFile *opts, const char *abi, tUdiprops *udiprops);

#endif

