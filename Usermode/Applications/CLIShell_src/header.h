/*
 Acess Shell Version 2
- Based on IOOS CLI Shell
*/
#ifndef _HEADER_H
#define _HEADER_H

#define Print(str)	do{char*s=(str);write(_stdout,strlen(s)+1,s);}while(0)

extern int _stdout;
extern int _stdin;

extern int	GeneratePath(char *file, char *base, char *tmpPath);


#endif
