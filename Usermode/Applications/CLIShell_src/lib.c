/*
 * AcessOS Shell Version 2
 *
 * See file COPYING for 
 */
#include "header.h"

// === CODE ===
/**
 * \fn int GeneratePath(char *file, char *base, char *Dest)
 * \brief Generates a path given a base and a filename
 * \param 
 */
int GeneratePath(char *file, char *base, char *Dest)
{
	char	*pathComps[64];
	char	*tmpStr;
	int		iPos = 0;
	int		iPos2 = 0;
	
	// Parse Base Path
	if(file[0] != '/')
	{
		pathComps[iPos++] = tmpStr = base+1;
		while(*tmpStr)
		{
			if(*tmpStr++ == '/')
			{
				pathComps[iPos] = tmpStr;
				iPos ++;
			}
		}
	}
	
	//Print(" GeneratePath: Base Done\n");
	
	// Parse Argument Path
	pathComps[iPos++] = tmpStr = file;
	while(*tmpStr)
	{
		if(*tmpStr++ == '/')
		{
			pathComps[iPos] = tmpStr;
			iPos ++;
		}
	}
	pathComps[iPos] = NULL;
	
	//Print(" GeneratePath: Path Done\n");
	
	// Cleanup
	iPos2 = iPos = 0;
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		// Always Increment iPos
		iPos++;
		// ..
		if(tmpStr[0] == '.' && tmpStr[1] == '.'	&& (tmpStr[2] == '/' || tmpStr[2] == '\0') )
		{
			if(iPos2 != 0)
				iPos2 --;
			continue;
		}
		// .
		if(tmpStr[0] == '.' && (tmpStr[1] == '/' || tmpStr[1] == '\0') )
		{
			continue;
		}
		// Empty
		if(tmpStr[0] == '/' || tmpStr[0] == '\0')
		{
			continue;
		}
		
		// Set New Position
		pathComps[iPos2] = tmpStr;
		iPos2++;
	}
	pathComps[iPos2] = NULL;
	
	// Build New Path
	iPos2 = 1;	iPos = 0;
	Dest[0] = '/';
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		while(*tmpStr && *tmpStr != '/')
		{
			Dest[iPos2++] = *tmpStr;
			tmpStr++;
		}
		Dest[iPos2++] = '/';
		iPos++;
	}
	if(iPos2 > 1)
		Dest[iPos2-1] = 0;
	else
		Dest[iPos2] = 0;
	
	return iPos2;	//Length
}
