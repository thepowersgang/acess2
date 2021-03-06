/*
 * Acess2 - URI Parser and opener
 * By John Hodge (thePowersGang)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <uri.h>

// === STRUCTURES ===
struct sURIFile
{
	void	*Handle;
	 int	Mode;
	tURIHandler	*Handler;
	 int	CurBlockOffset;
	char	Buffer[];
};

// === PROTOTYPES ===
 int	SoMain(void);
tURI	*URI_Parse(const char *String);
tURIFile	*URI_MakeHandle(int Mode, void *Handle, tURIHandler *Handler);
tURIFile	*URI_Open(int Mode, tURI *URI);
size_t	URI_Read(tURIFile *File, size_t Bytes, void *Buffer);
size_t	URI_Write(tURIFile *File, size_t Bytes, void *Buffer);
void	URI_Close(tURIFile *File);
// --- file:/// handler
void	*URI_file_Open(const char *Host, int Port, const char *Path, int Mode);
void	URI_file_Close(void *Handle);
size_t	URI_file_Read(void *Handle, size_t Bytes, void *Buffer);
size_t	URI_file_Write(void *Handle, size_t Bytes, const void *Buffer);
off_t	URI_file_GetSize(void *Handle);

// === CONSTANTS ===
// Builtin URI protocol handlers
tURIHandler	caBuiltinHandlers[] = {
	{"file", 0,
		URI_file_Open, URI_file_Close,
		URI_file_Read, URI_file_Write,
		URI_file_GetSize
	}
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
		 int	hostlen, portlen;
		tmp += 3;	// Eat '://'
		ret = malloc(sizeof(tURI) + protolen + 1 + strlen(tmp) + 1);
		
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
		
		strcpy(ret->Path, tmp);
		
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

tURIFile *URI_MakeHandle(int Mode, void *Handle, tURIHandler *Handler)
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
	void	*handle;
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
	if(handle == NULL)	return NULL;
	
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
void *URI_file_Open(const char *Host, int Port, const char *Path, int Mode)
{
	const char	*smode = NULL;
	if( (Mode & URI_MODE_READ) && (Mode & URI_MODE_WRITE))
		smode = "r+";
	else if( (Mode & URI_MODE_READ) )
		smode = "r";
	else if( (Mode & URI_MODE_WRITE) )
		smode = "w";
	else
		smode = "";
	
//	printf("URI_file_Open: open('%s', 0x%x)\n", Path, smode);
	return fopen(Path, smode);
}
size_t URI_file_Read(void *Handle, size_t Bytes, void *Buffer)
{
//	printf("URI_file_Read: (Handle=%i, Buffer=%p, Bytes=%i)\n",
//		Handle, Buffer, (int)Bytes);
	return fread(Buffer, Bytes, 1, Handle);
}
size_t URI_file_Write(void *Handle, size_t Bytes, const void *Buffer)
{
	return fwrite(Buffer, Bytes, 1, Handle);
}
void URI_file_Close(void *Handle)
{
	fclose(Handle);
}
off_t URI_file_GetSize(void *Handle)
{
	off_t curpos = ftell(Handle);
	off_t ret;
	fseek(Handle, 0, SEEK_END);
	ret = ftell(Handle);
	fseek(Handle, curpos, SEEK_SET);
	return ret;
}

