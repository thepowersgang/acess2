/*
 * Acess2 - URI Parser and opener
 * By John Hodge (thePowersGang)
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uri.h>

// === STRUCTURES ===
struct sURIFile
{
	 int	Handle;
	 int	Mode;
	tURIHandler	*Handler;
	 int	CurBlockOffset;
	char	Buffer[];
};

// === PROTOTYPES ===
 int	SoMain(void);
tURI	*URI_Parse(const char *String);
tURIFile	*URI_MakeHandle(int Mode, int Handle, tURIHandler *Handler);
tURIFile	*URI_Open(int Mode, tURI *URI);
size_t	URI_Read(tURIFile *File, size_t Bytes, void *Buffer);
size_t	URI_Write(tURIFile *File, size_t Bytes, void *Buffer);
void	URI_Close(tURIFile *File);
// --- file:/// handler
 int	URI_file_Open(char *Host, int Port, char *Path, int Mode);
size_t	URI_file_Read(int Handle, size_t Bytes, void *Buffer);
size_t	URI_file_Write(int Handle, size_t Bytes, void *Buffer);
void	URI_file_Close(int Handle);
off_t	URI_file_GetSize(int Handle);

// === CONSTANTS ===
// Builtin URI protocol handlers
tURIHandler	caBuiltinHandlers[] = {
	{"file", 0, URI_file_Open, URI_file_Close, URI_file_Read, URI_file_Write, URI_file_GetSize}
};
#define NUM_BUILTIN_HANDLERS	(sizeof(caBuiltinHandlers)/sizeof(caBuiltinHandlers[0]))

// === CODE ===
int SoMain(void)
{
	return 0;
}

tURI *URI_Parse(const char *String)
{
	const char	*tmp = String;
	tURI	*ret;
	 int	protolen;
	
	if(!String)	return NULL;
	
	protolen = 0;
	while( isalpha(*tmp) || isdigit(*tmp) )	tmp++, protolen++;
	
	// true URI
	if(tmp[0] == ':' && tmp[1] == '/' && tmp[2] == '/')
	{
		 int	hostlen, portlen, pathlen;
		tmp += 3;	// Eat '://'
		ret = malloc(sizeof(tURI) + strlen(String) - 2);
		
		ret->Proto = (char*)ret + sizeof(tURI);
		memcpy(ret->Proto, String, protolen);
		ret->Proto[protolen] = '\0';
		
		ret->Host = ret->Proto + protolen + 1;
		hostlen = 0;
		
		// IPv6
		if( *tmp == '[' )
		{
			tmp ++;
			while( *tmp && *tmp != ']' ) {
				ret->Host[hostlen] = *tmp;
				tmp ++;
				hostlen ++;
			}
			tmp ++;
			ret->Host[hostlen] = '\0';
		}
		// IPv4/DNS
		else
		{
			while( *tmp && *tmp != '/' && *tmp != ':' )
			{
				ret->Host[hostlen] = *tmp;
				tmp ++;
				hostlen ++;
			}
			ret->Host[hostlen] = '\0';
		}
		
		// Port
		if( *tmp == ':' ) {
			ret->PortStr = ret->Host + hostlen + 1;
			tmp ++;
			portlen = 0;
			while(isalpha(*tmp) || isdigit(*tmp))
			{
				ret->PortStr[portlen] = *tmp;
				portlen ++;
				tmp ++;
			}
			ret->PortStr[portlen] = '\0';
			
			ret->PortNum = atoi(ret->PortStr);
			if(!ret->PortNum && !(ret->PortStr[0] == '0' && portlen == 1) )
			{
				// error!
				ret->Path = NULL;
				return ret;
			}
		}
		else {
			ret->PortStr = NULL;
			ret->PortNum = -1;
			portlen = 0;
		}
		
		if(*tmp == '\0')
		{
			ret->Path = NULL;
			return ret;
		}
		
		// TODO: What to do on a parse error
		if(*tmp != '/') {
			ret->Path = NULL;
			return ret;
		}
		
		if(ret->PortStr)
			ret->Path = ret->PortStr + portlen + 1;
		else
			ret->Path = ret->Host + hostlen + 1;
		
		tmp ++;
		pathlen = 0;
		while(*tmp)
		{
			ret->Path[pathlen] = *tmp;
			tmp ++;
			pathlen ++;
		}
		
		return ret;
	}
	else
	{
//		char	*cwd;
//		 int	retlen;
		
		// Path
		// TODO: What to do?
		// Probably return file:///<path>
		// but should I get the CWD and use append that?
		ret = malloc( sizeof(tURI) + strlen(String) + 1 );
		ret->Path = (char*)ret + sizeof(tURI);
		strcpy(ret->Path, String);
		ret->Proto = "file";
		ret->Host = NULL;
		ret->PortNum = 0;
		ret->PortStr = NULL;
		return ret;
	}
}

tURIFile *URI_MakeHandle(int Mode, int Handle, tURIHandler *Handler)
{
	tURIFile	*ret;
	
	ret = malloc(sizeof(tURIFile)+Handler->BlockSize);
	if(!ret)	return NULL;
	
	ret->Handle = Handle;
	ret->Mode = Mode;
	ret->Handler = Handler;
	ret->CurBlockOffset = 0;
	
	return ret;
}

tURIFile *URI_Open(int Mode, tURI *URI)
{
	tURIHandler	*handler;
	tURIFile	*ret;
	 int	handle;
	 int	i;
	
	if(!URI)
		return NULL;
	
	for( i = 0; i < NUM_BUILTIN_HANDLERS; i ++ )
	{
		if(strcmp(URI->Proto, caBuiltinHandlers[i].Name) == 0)
			break;
	}
	
	if( i == NUM_BUILTIN_HANDLERS )
	{
		// TODO: Dynamics
		printf("URI_Open: Warning - Unknown URI handler\n");
		return NULL;
	}
	else
		handler = &caBuiltinHandlers[i];
	
	printf("URI_Open: handler->Open = %p\n", handler->Open);
	
	handle = handler->Open(URI->Host, URI->PortNum, URI->Path, Mode);
	printf("URI_Open: handle = %i\n", handle);
	if(handle == -1)	return NULL;
	
	printf("URI_MakeHandle(Mode=%i, handle=%i, handler=%p)\n",
		Mode, handle, handler);
	ret = URI_MakeHandle(Mode, handle, handler);
	if(!ret) {
		handler->Close( handle );
		return NULL;
	}
	return ret;
}

int URI_GetSize(tURIFile *File, size_t *Size)
{
	if( !File || !Size )	return -1;
	
	if( File->Handler->GetSize )
	{
		*Size = File->Handler->GetSize(File->Handle);
		return 0;	// Success
	}
	
	return 1;	// Size not avaliable
}

/**
 * \brief Read from a URI file
 */
size_t URI_Read(tURIFile *File, size_t Bytes, void *Buffer)
{
	size_t	rem = Bytes;
	void	*buf = Buffer;
	size_t	tmp;
	
	printf("URI_Read(File=%p, Bytes=%u, Buffer=%p)\n",
		File, (unsigned int)Bytes, Buffer);
	
	if(!File || !Buffer)	return -1;
	if(Bytes == 0)	return 0;
	
	if( !(File->Mode & URI_MODE_READ) )	return -1;
	
	// Read from cache if avaliable
	if(File->Handler->BlockSize && File->CurBlockOffset)
	{
		 int	avail;
		
		avail = File->Handler->BlockSize - File->CurBlockOffset;
		
		if(avail >= Bytes) {
			memcpy(Buffer, File->Buffer, Bytes);
			File->CurBlockOffset += Bytes;
			File->CurBlockOffset %= File->Handler->BlockSize;
			return Bytes;
		}
		
		rem -= avail;
		memcpy(Buffer, File->Buffer, avail);
		File->CurBlockOffset = 0;
		buf += avail;
	}
	
	
	if( File->Handler->BlockSize )
	{
		// Read whole blocks
		while( rem >= File->Handler->BlockSize )
		{
			tmp = File->Handler->Read( File->Handle, File->Handler->BlockSize, buf );
			if(tmp < File->Handler->BlockSize)
				return Bytes - rem - tmp;
			buf += File->Handler->BlockSize;
		}
		
		// Read the trailing part
		if(rem)
		{
			File->Handler->Read( File->Handle, File->Handler->BlockSize, File->Buffer );
			memcpy( buf, File->Buffer, rem );
			File->CurBlockOffset += rem;
		}
		return Bytes;
	}
	
	return File->Handler->Read( File->Handle, Bytes, Buffer );
}

/**
 * \brief Write to a URI file
 */


// ====
// Builtin Handlers
// ====
int URI_file_Open(char *Host, int Port, char *Path, int Mode)
{
	 int	smode = 0;
	if(Mode & URI_MODE_READ)	smode |= OPENFLAG_READ;
	if(Mode & URI_MODE_WRITE)	smode |= OPENFLAG_WRITE;
	
//	printf("URI_file_Open: open('%s', 0x%x)\n", Path, smode);
	{
		 int	ret;
		ret = _SysOpen(Path, smode);
		return ret;
	}
}
size_t URI_file_Read(int Handle, size_t Bytes, void *Buffer)
{
//	printf("URI_file_Read: (Handle=%i, Buffer=%p, Bytes=%i)\n",
//		Handle, Buffer, (int)Bytes);
	return _SysRead(Handle, Buffer, Bytes);
}
size_t URI_file_Write(int Handle, size_t Bytes, void *Buffer)
{
	return _SysWrite(Handle, Buffer, Bytes);
}
void URI_file_Close(int Handle)
{
	_SysClose(Handle);
}
off_t URI_file_GetSize(int Handle)
{
	uint64_t curpos = _SysTell(Handle);
	off_t ret;
	_SysSeek(Handle, 0, SEEK_END);
	ret = _SysTell(Handle);
	_SysSeek(Handle, curpos, SEEK_SET);
	return ret;
}
