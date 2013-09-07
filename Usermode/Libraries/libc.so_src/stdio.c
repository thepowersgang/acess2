/*
 * AcessOS Basic C Library
 * stdio.c
 */
#include "config.h"
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib.h"
#include "stdio_int.h"
#include <errno.h>
#include <assert.h>

#define DEBUG_BUILD	0

// === CONSTANTS ===
#define	_stdin	0
#define	_stdout	1

#define FD_NOTOPEN	-1
#define FD_MEMFILE	-2
#define FD_MEMSTREAM	-3

// === PROTOTYPES ===
struct sFILE	*get_file_struct();

// === GLOBALS ===
struct sFILE	_iob[STDIO_MAX_STREAMS];	// IO Buffer
struct sFILE	*stdin;	// Standard Input
struct sFILE	*stdout;	// Standard Output
struct sFILE	*stderr;	// Standard Error
///\note Initialised in SoMain
static const int STDIN_BUFSIZ = 512;
static const int STDOUT_BUFSIZ = 512;

// === CODE ===
void _stdio_init(void)
{
	// Init FileIO Pointers
	stdin = &_iob[0];
	stdin->FD = 0;
	stdin->Flags = FILE_FLAG_ALLOC|FILE_FLAG_MODE_READ|FILE_FLAG_LINEBUFFERED;
	stdin->Buffer = malloc(STDIN_BUFSIZ);
	stdin->BufferSpace = STDIN_BUFSIZ;

	stdout = &_iob[1];
	stdout->FD = 1;
	stdout->Flags = FILE_FLAG_ALLOC|FILE_FLAG_MODE_WRITE|FILE_FLAG_LINEBUFFERED;
	stdout->Buffer = malloc(STDOUT_BUFSIZ);
	stdout->BufferSpace = STDOUT_BUFSIZ;
	
	stderr = &_iob[2];
	stderr->FD = 2;
	stderr->Flags = FILE_FLAG_ALLOC|FILE_FLAG_MODE_WRITE;
}

void _stdio_cleanup(void)
{
	 int	i;
	for( i = 0; i < STDIO_MAX_STREAMS; i ++ )
	{
		fclose( &_iob[i] );
	}
}

int _fopen_modetoflags(const char *mode)
{
	int flags = 0;
	
	// Get main mode
	switch(*mode)
	{
	case 'r':	flags = FILE_FLAG_MODE_READ;	break;
	case 'w':	flags = FILE_FLAG_MODE_WRITE;	break;
	case 'a':	flags = FILE_FLAG_MODE_APPEND;	break;
	case 'x':	flags = FILE_FLAG_MODE_EXEC;	break;	// Acess addon
	default:
		return -1;
	}
	mode ++;

	// Get Modifiers
	for( ; *mode; mode ++ )
	{
		switch(*mode)
		{
		case 'b':	flags |= FILE_FLAG_M_BINARY;	break;
		case '+':	flags |= FILE_FLAG_M_EXT;	break;
		default:
			return -1;
		}
	}
	
	return flags;
}

/**
 * \fn FILE *freopen(char *file, char *mode, FILE *fp)
 */
EXPORT FILE *freopen(const char *file, const char *mode, FILE *fp)
{
	 int	openFlags = 0;
	
	// Sanity Check Arguments
	if(!fp || !file || !mode)	return NULL;
	
	if(fp->FD != FD_NOTOPEN) {
		fflush(fp);
	}

	// Get stdio flags
	fp->Flags = _fopen_modetoflags(mode);
	if(fp->Flags == -1)
		return NULL;
	
	// Get Open Flags
	switch(fp->Flags & FILE_FLAG_MODE_MASK)
	{
	// Read
	case FILE_FLAG_MODE_READ:
		openFlags = OPENFLAG_READ;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_WRITE;
		break;
	// Write
	case FILE_FLAG_MODE_WRITE:
		openFlags = OPENFLAG_WRITE;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_READ;
		break;
	// Execute
	case FILE_FLAG_MODE_APPEND:
		openFlags = OPENFLAG_APPEND;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_READ;
		break;
	// Execute
	case FILE_FLAG_MODE_EXEC:
		openFlags = OPENFLAG_EXEC;
		break;
	}

	//Open File
	if(fp->FD != FD_NOTOPEN)
		fp->FD = _SysReopen(fp->FD, file, openFlags);
	else
		fp->FD = _SysOpen(file, openFlags);
	if(fp->FD == -1) {
		fp->Flags = 0;
		return NULL;
	}
	
	if( (fp->Flags & FILE_FLAG_MODE_MASK) == FILE_FLAG_MODE_APPEND ) {
		_SysSeek(fp->FD, 0, SEEK_END);	//SEEK_END
	}
	
	return fp;
}
/**
 \fn FILE *fopen(const char *file, const char *mode)
 \brief Opens a file and returns the pointer
 \param file	String - Filename to open
 \param mode	Mode to open in
*/
EXPORT FILE *fopen(const char *file, const char *mode)
{
	FILE	*retFile;
	
	// Sanity Check Arguments
	if(!file || !mode)	return NULL;
	
	// Create Return Structure
	retFile = get_file_struct();
	
	return freopen(file, mode, retFile);
}

EXPORT FILE *fmemopen(void *buffer, size_t length, const char *mode)
{
	FILE	*ret;
	
	if( !buffer || !mode )	return NULL;
	
	ret = get_file_struct();
	
	ret->FD = FD_MEMFILE;
	ret->Flags = _fopen_modetoflags(mode);
	if(ret->Flags == -1) {
		ret->Flags = 0;
		return NULL;
	}
	
	ret->Buffer = buffer;
	ret->BufferPos = 0;
	ret->BufferSpace = length;
	
	return ret;
}

EXPORT FILE *open_memstream(char **bufferptr, size_t *lengthptr)
{
	FILE	*ret = get_file_struct();
	ret->FD = FD_MEMSTREAM;
	ret->Flags = FILE_FLAG_MODE_WRITE;
	
	ret->Buffer = NULL;
	ret->BufferPos = 0;
	ret->BufferSpace = 0;
	ret->BufPtr = bufferptr;
	ret->LenPtr = lengthptr;
	
	return ret;
}

EXPORT int fclose(FILE *fp)
{
	if( !(fp->Flags & FILE_FLAG_ALLOC) )
		return 0;
	fflush(fp);
	if( fp->FD >= 0 ) {
		_SysClose(fp->FD);
	}
	fp->Flags = 0;
	fp->FD = FD_NOTOPEN;
	return 0;
}

EXPORT int setvbuf(FILE *fp, char *buffer, int mode, size_t size)
{
	if( !fp ) {
		errno = EINVAL;
		return 1;
	}

	// Check for memory files
	if( fp->FD == FD_MEMFILE || fp->FD == FD_MEMSTREAM ) {
		errno = EINVAL;
		return 2;
	}	

	// Not strictly needed, as this should only be called before any IO
	fflush(fp);

	// Eliminate any pre-existing buffer
	if( fp->Buffer ) {
		free( fp->Buffer );
		fp->Buffer = NULL;
		fp->BufferSpace = 0;
		fp->BufferPos = 0;
	}

	if( mode == _IONBF ) {
		// Do nothing, buffering was disabled above
	}
	else
	{
		// Sanity check buffering mode before allocating
		if( mode != _IOLBF && mode != _IOFBF ) {
			errno = EINVAL;
			return 1;
		}
		// Allocate a buffer if one was not provided
		if( !buffer ) {
			buffer = malloc(size);
			assert(buffer);
		}
		
		// Set buffer pointer and size
		fp->Buffer = buffer;
		fp->BufferSpace = size;
		
		// Set mode flag
		if( mode == _IOLBF )
			fp->Flags |= FILE_FLAG_LINEBUFFERED;
		else
			fp->Flags &= ~FILE_FLAG_LINEBUFFERED;
	}
	
	return 0;
}

int _fflush_int(FILE *fp)
{
	 int	ret = 0;
	size_t	len;
	
	// Check the buffer contains data
	if( fp->BufferPos == 0 )
		return 0;
	
	switch(fp->Flags & FILE_FLAG_MODE_MASK)
	{
	// Read - Flush input buffer
	case FILE_FLAG_MODE_READ:
		fp->BufferPos = 0;
		break;
	
	// Append - Seek to end and write
	case FILE_FLAG_MODE_APPEND:
		_SysSeek(fp->FD, fp->BufferOfs, SEEK_SET);
		len = _SysWrite(fp->FD, fp->Buffer, fp->BufferPos);
		if( len != fp->BufferPos )
			ret = 1;
		if( len <= fp->BufferPos )
		{
			fp->BufferPos -= len;
		}
		fp->BufferOfs = _SysTell(fp->FD);
		break;
		
	// Write - Write buffer
	case FILE_FLAG_MODE_WRITE:
		//_SysDebug("Flushing to %i '%.*s'", fp->FD, fp->BufferPos, fp->Buffer);
		len = _SysWrite(fp->FD, fp->Buffer, fp->BufferPos);
		if( len != fp->BufferPos )
			ret = 1;
		if( len <= fp->BufferPos )
		{
			fp->BufferPos -= len;
		}
		//else {
		//	_SysDebug("Flush of %i failed, %s", fp->FD, strerror(errno));
		//}
		break;
	default:
		break;
	}
	return ret;
}

EXPORT void fflush(FILE *fp)
{
	if( !fp || fp->FD == FD_NOTOPEN )
		return ;
	
	// Nothing to do for memory files
	if( fp->FD == FD_MEMFILE )
		return ;
	// Memory streams, update pointers
	if( fp->FD == FD_MEMSTREAM ) {
		*fp->BufPtr = fp->Buffer;
		*fp->LenPtr = fp->BufferPos;
		return ;
	}
	
	_fflush_int(fp);
}

EXPORT void clearerr(FILE *fp)
{
	if( !fp || fp->FD == FD_NOTOPEN )
		return ;
	
	// TODO: Impliment clearerr()
}

EXPORT int feof(FILE *fp)
{
	if( !fp || fp->FD == FD_NOTOPEN )
		return 0;
	return !!(fp->Flags & FILE_FLAG_EOF);
}

EXPORT int ferror(FILE *fp)
{
	if( !fp || fp->FD == FD_NOTOPEN )
		return 0;
	return 0;
}
EXPORT int fileno(FILE *stream)
{
	return stream->FD;
}

EXPORT off_t ftell(FILE *fp)
{
	if(!fp || fp->FD == FD_NOTOPEN) {
		errno = EBADF;
		return -1;
	}

	if( fp->FD == FD_MEMFILE || fp->FD == FD_MEMSTREAM )
		return fp->Pos;	
	else
		return _SysTell(fp->FD);
}

int _fseek_memfile(FILE *fp, long int amt, int whence)
{
	switch(whence)
	{
	case SEEK_CUR:
		fp->Pos += amt;
		break;
	case SEEK_SET:
		fp->Pos = amt;
		break;
	case SEEK_END:
		if( fp->BufferSpace < (size_t)amt )
			fp->Pos = 0;
		else
			fp->Pos = fp->BufferSpace - amt;
		break;
	}
	if(fp->Pos > (off_t)fp->BufferSpace) {
		fp->Pos = fp->BufferSpace;
		fp->Flags |= FILE_FLAG_EOF;
	}
	return 0;
}

int _fseek_memstream(FILE *fp, long int amt, int whence)
{
	switch(whence)
	{
	case SEEK_CUR:
		fp->Pos += amt;
		break;
	case SEEK_SET:
		fp->Pos = amt;
		break;
	case SEEK_END:
		if( fp->BufferSpace < (size_t)amt )
			fp->Pos = 0;
		else
			fp->Pos = fp->BufferSpace - amt;
		break;
	}
	if(fp->Pos > (off_t)fp->BufferSpace) {
		fp->Pos = fp->BufferSpace;
		fp->Flags |= FILE_FLAG_EOF;
	}
	return 0;
}

EXPORT int fseek(FILE *fp, long int amt, int whence)
{
	if(!fp || fp->FD == FD_NOTOPEN) {
		errno = EBADF;
		return -1;
	}

	if( fp->FD == FD_MEMFILE ) {
		return _fseek_memfile(fp, amt, whence);
	}
	else if( fp->FD == FD_MEMSTREAM ) {
		return _fseek_memstream(fp, amt, whence);
	}
	else {
		fflush(fp);
		_SysSeek(fp->FD, amt, whence);
		fp->Pos = _SysTell(fp->FD);
		return 0;
	}
}

size_t _fwrite_unbuffered(FILE *fp, size_t size, size_t num, const void *data)
{
	size_t	ret = 0, bytes;
	while( num -- )
	{
		bytes = _SysWrite(fp->FD, data, size);
		if( bytes == (size_t)-1 ) {
			// Oops.
			// TODO: Set error flag
			break;
		}
		if( bytes != size ) {
			_SysDebug("_fwrite_unbuffered: Oops, rollback %i/%i bytes!", bytes, size);
			_SysSeek(fp->FD, -bytes, SEEK_CUR);
			break;
		}
		data = (char*)data + size;
	}
	fp->Pos += ret * size;
	return ret;
}

size_t _fwrite_memfile(const void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	avail = (fp->BufferSpace - fp->Pos) / size;
	if( avail == 0 )
		fp->Flags |= FILE_FLAG_EOF;
	if( num > avail )
		num = avail;
	size_t	bytes = num * size;
	memcpy(fp->Buffer + fp->Pos, ptr, bytes);
	fp->Pos += bytes;
	return num;
}

size_t _fwrite_memstream(const void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	bytes = size*num;
	// #1. Check if we need to expand
	if( fp->Pos + bytes > fp->BufferSpace )
	{
		void *newbuf = realloc(fp->Buffer, fp->BufferSpace + bytes);
		if( !newbuf ) {
			errno = ENOMEM;
			return -1;
		}
		fp->Buffer = newbuf;
		fp->BufferSpace = fp->Pos + bytes;
	}
	// #2. Write (use the memfile code for that)
	return _fwrite_memfile(ptr, size, num, fp);
}

/**
 * \fn EXPORT size_t fwrite(void *ptr, size_t size, size_t num, FILE *fp)
 * \brief Write to a stream
 */
EXPORT size_t fwrite(const void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	ret;
	
	if(!fp || fp->FD == -1)
		return -1;
	if( size == 0 || num == 0 )
		return 0;

	switch( _GetFileMode(fp) )
	{
	case FILE_FLAG_MODE_READ:
	case FILE_FLAG_MODE_EXEC:
		errno = EBADF;
		return 0;
	case FILE_FLAG_MODE_APPEND:
		fseek(fp, 0, SEEK_END);
	case FILE_FLAG_MODE_WRITE:
		// Handle memory files first
		if( fp->FD == FD_MEMFILE ) {
			return _fwrite_memfile(ptr, size, num, fp);
		}
		else if( fp->FD == FD_MEMSTREAM ) {
			return _fwrite_memstream(ptr, size, num, fp);
		}
		else if( fp->BufferSpace )
		{
			// Buffering enabled
			if( fp->BufferSpace - fp->BufferPos < size*num )
			{
				// If there's not enough space, flush and write-through
				_fflush_int(fp);	// TODO: check ret
				ret = _fwrite_unbuffered(fp, size, num, ptr);
			}
			else if( (fp->Flags & FILE_FLAG_LINEBUFFERED) && memchr(ptr,'\n',size*num) )
			{
				// Newline present? Flush though
				_fflush_int(fp);	// TODO: check ret
				ret = _fwrite_unbuffered(fp, size, num, ptr);
			}
			else
			{
				// Copy to buffer
				memcpy( fp->Buffer + fp->BufferPos, ptr, size*num );
				fp->BufferPos += size*num;
				ret = num;
			}
		}
		else
		{
			// Bufering disabled, write-though
			ret = _fwrite_unbuffered(fp, size, num, ptr);
		}
		// errno should be set earlier
		break;
	}
	
	return ret;
}

size_t _fread_memfile(void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	avail = (fp->BufferSpace - fp->Pos) / size;
	if( avail == 0 )
		fp->Flags |= FILE_FLAG_EOF;
	if( num > avail )	num = avail;
	size_t	bytes = num * size;
	memcpy(ptr, fp->Buffer + fp->Pos, bytes);
	fp->Pos += bytes;
	return num;
}

#if 0
size_t _fread_memstream(void *ptr, size_t size, size_t num, FILE *fp)
{
	errno = ENOTIMPL;
	return -1;
}
#endif

/**
 * \fn EXPORT size_t fread(void *ptr, size_t size, size_t num, FILE *fp)
 * \brief Read from a stream
 */
EXPORT size_t fread(void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	ret;
	
	if(!fp || fp->FD == -1)
		return -1;
	if( size == 0 || num == 0 )
		return 0;
	
	if( _GetFileMode(fp) != FILE_FLAG_MODE_READ ) {
		errno = 0;
		return -1;
	}

	if( fp->FD == FD_MEMFILE ) {
		return _fread_memfile(ptr, size, num, fp);
	}
	else if( fp->FD == FD_MEMSTREAM ) {
		//return _fread_memstream(ptr, size, num, fp);
		errno = EBADF;
		return 0;
	}
	
	// Standard file
	ret = _SysRead(fp->FD, ptr, size*num);
	if( ret == (size_t)-1)
		return -1;
	if( ret == 0 && size*num > 0 ) {
		fp->Flags |= FILE_FLAG_EOF;
		return 0;
	}
	ret /= size;
	
	return ret;
}

/**
 * \brief Write a string to a stream (without trailing \n)
 */
EXPORT int fputs(const char *s, FILE *fp)
{
	int len = strlen(s);
	return fwrite(s, 1, len, fp);
}

/**
 * \brief Read a line (and possible trailing \n into a buffer)
 */
EXPORT char *fgets(char *s, int size, FILE *fp)
{
	int ofs = 0;
	char	ch = '\0';
	while( ofs < size && ch != '\n' )
	{
		if( fread(&ch, 1, 1, fp) != 1 )
			break;
		s[ofs ++] = ch;
	}
	if( ofs < size )
		s[ofs] = '\0';
	return s;
}

/**
 * \fn EXPORT int fputc(int c, FILE *fp)
 * \brief Write a single character to the stream
 */
EXPORT int fputc(int c, FILE *fp)
{
	return fwrite(&c, 1, 1, fp);
}

EXPORT int putchar(int c)
{
	c &= 0xFF;
	return _SysWrite(_stdout, &c, 1);
}

/**
 * \fn EXPORT int fgetc(FILE *fp)
 * \brief Read a character from the stream
 */
EXPORT int fgetc(FILE *fp)
{
	char	ret = 0;
	if( fread(&ret, 1, 1, fp) != 1 )
		return -1;
	return ret;
}

EXPORT int getchar(void)
{
	char	ret = 0;
	if(_SysRead(_stdin, &ret, 1) != 1)	return -1;
	return ret;
}

EXPORT int puts(const char *str)
{
	 int	len;
	
	if(!str)	return 0;
	len = strlen(str);
	
	len = _SysWrite(_stdout, str, len);
	_SysWrite(_stdout, "\n", 1);
	return len;
}

// --- INTERNAL ---
/**
 * \fn FILE *get_file_struct()
 * \brief Returns a file descriptor structure
 */
FILE *get_file_struct()
{
	 int	i;
	for(i=0;i<STDIO_MAX_STREAMS;i++)
	{
		if(_iob[i].Flags & FILE_FLAG_ALLOC)
			continue ;
		_iob[i].Flags |= FILE_FLAG_ALLOC;
		_iob[i].FD = -1;
		_iob[i].Pos = 0;
		return &_iob[i];
	}
	return NULL;
}

