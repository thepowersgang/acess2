/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * inifile.c
 * - .ini file parsing
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "include/inifile.h"

typedef struct sInifile_Section	tIniFile_Section;
typedef struct sInifile_Value	tIniFile_Value;

struct sInifile_Value
{
	tIniFile_Value	*Next;
	const char	*Key;
	const char	*Value;
};

struct sInifile_Section
{
	tIniFile_Section	*Next;
	const char *Name;
	tIniFile_Value	*FirstValue;
};

struct sInifile
{
	tIniFile_Section	RootSection;
};

// === CODE ===
static void rtrim(char *str)
{
	char *pos = str;
	while( *pos )
		pos ++;
	while( pos != str && isspace(pos[-1]) )
		*--pos = '\0';
}

tIniFile *IniFile_Load(const char *Path)
{
	FILE	*fp = fopen(Path, "r");
	if( !fp )
		return NULL;
	
	tIniFile	*ret = malloc( sizeof(tIniFile) );
	assert(ret);

	ret->RootSection.Name = "";
	ret->RootSection.FirstValue = NULL;

	tIniFile_Section	*curSect = &ret->RootSection;
	char buf[512];
	while( fgets(buf, sizeof(buf)-1, fp) )
	{
		if( strchr(buf, '#') )
			*strchr(buf, '#') = '\0';
		rtrim(buf);
		char name[64];
		size_t ofs = 0;
		if( sscanf(buf, "[%[^]]]", name) == 1 ) {
			//printf("section %s\n", name);
			// new section
			tIniFile_Section *new_sect = malloc(sizeof(tIniFile_Section)+strlen(name)+1);
			new_sect->Next = NULL;
			new_sect->Name = (const char*)(new_sect+1);
			new_sect->FirstValue = NULL;
			strcpy( (char*)new_sect->Name, name );
			curSect->Next = new_sect;
			curSect = new_sect;
		}
		else if( sscanf(buf, "%[^=]=%n", name, &ofs) >= 1 ) {
			//printf("key %s equals %s\n", name, value);
			const char *value = buf + ofs;
			tIniFile_Value *val = malloc(sizeof(tIniFile_Value)+strlen(name)+1+strlen(value)+1);
			val->Next = curSect->FirstValue;
			curSect->FirstValue = val;
			
			val->Key = (char*)(val+1);
			strcpy((char*)val->Key, name);
			val->Value = val->Key + strlen(val->Key) + 1;
			strcpy((char*)val->Value, value);
		}
		else {
			//printf("ignore %s\n", buf);
			// ignore
		}
	}

	fclose(fp);	

	return ret;
}

const char *IniFile_Get(tIniFile *File, const char *Sect, const char *Key, const char *Default)
{
	tIniFile_Section	*sect;
	for( sect = &File->RootSection; sect; sect = sect->Next )
	{
		if( strcmp(sect->Name, Sect) == 0 )
			break;
	}
	if( !sect )
		return Default;
	
	tIniFile_Value	*val;
	for( val = sect->FirstValue; val; val = val->Next )
	{
		if( strcmp(val->Key, Key) == 0 )
			break;
	}
	if( !val )
		return Default;
	
	return val->Value;
}

void IniFile_Free(tIniFile *File)
{
	// TODO:
}
