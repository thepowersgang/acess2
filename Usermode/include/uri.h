/*
 * Acess2 - URI Parser and opener
 * By John Hodge (thePowersGang)
 */
#ifndef _LIB_URI_H_
#define _LIB_URI_H_

typedef struct sURI	tURI;
typedef struct sURIFile	tURIFile;
typedef struct sURIHandler	tURIHandler;

enum eURIModes
{
	URI_MODE_READ  = 0x01,
	URI_MODE_WRITE = 0x02
};

struct sURI
{
	char	*Proto;
	char	*Host;
	char	*PortStr;
	 int	PortNum;
	char	*Path;
};

struct sURIHandler
{
	char	*Name;
	 int	BlockSize;
	
	 int	(*Open)(char *Host, int Port, char *Path, int Flags);
	void	(*Close)(int Handle);
	size_t	(*Read)(int Handle, size_t Bytes, void *Buffer);
	size_t	(*Write)(int Handle, size_t Bytes, void *Buffer);
	size_t	(*GetSize)(int Handle);
};

// === FUNCTIONS ===
extern tURI	*URI_Parse(const char *String);
extern tURIFile	*URI_Open(int Mode, tURI *URI);
extern int	URI_GetSize(tURIFile *File, size_t *Size);
extern size_t	URI_Read(tURIFile *File, size_t Bytes, void *Buffer);
extern size_t	URI_Write(tURIFile *File, size_t Bytes, void *Buffer);
extern void	URI_Close(tURIFile *File);

#endif
