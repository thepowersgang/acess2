/*
 * Acess2 Text Editor
 */
#if !USE_LOCAL
# include <acess/sys.h>	// Needed for IOCtl
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#if USE_LOCAL
# include <sys/ioctl.h>
# include <termios.h>
#else
# include <acess/devices/terminal.h>
#endif

#define MAX_FILES_OPEN	1

// === TYPES ===
typedef struct
{
	const char	*Filename;
	char	**LineBuffer;
	 int	FileSize;
	 int	LineCount;
	 
	 int	FirstLine;
	 int 	CurrentLine;	//!< Currently selected line
	 int 	CurrentPos;	//!< Position in that line
}	tFile;

// === PROTOTYPES ===
void	SigINT_Handler(int Signum);
void	ExitHandler(void);
 int	main(int argc, char *argv[]);
 int	OpenFile(tFile *Dest, const char *Path);
void	UpdateDisplayFull(void);
void	UpdateDisplayLine(int LineNum);
void	UpdateDisplayStatus(void);

void	CursorUp(tFile *File);
void	CursorDown(tFile *File);
void	CursorRight(tFile *File);
void	CursorLeft(tFile *File);

void	Term_SetPos(int Row, int Col);
void	Term_ScrollUp(int Count);

// === GLOBALS ===
 int	giProgramState = 0;
tFile	gaFiles[MAX_FILES_OPEN];
tFile	*gpCurrentFile;
#if USE_LOCAL
struct termios	gOldTermios;
#endif
 int	giTabSize = 8;	// Someday this will be per-file (maybe)
 int	giTerminal_Width = 80;
 int	giTerminal_Height = 25;

// === CODE ===
void SigINT_Handler(int Signum)
{
	exit(55);
}

void ExitHandler(void)
{
	#if USE_LOCAL
	tcsetattr(0, TCSANOW, &gOldTermios);
	#endif
	printf("\x1B[?1047l");
}

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	
	if(argc < 2) {
		printf("Usage: edit <file>\n");
		return -1;
	}

	if( OpenFile(&gaFiles[0], argv[1]) ) {
		return -1;
	}
	
	signal(SIGINT, SigINT_Handler);
	atexit(ExitHandler);

	// Disable input buffering
	#if USE_LOCAL
	{
		struct termios	term;
		tcgetattr(0, &gOldTermios);
		term = gOldTermios;
		term.c_lflag &= ~(ICANON|ECHO);
		tcsetattr(0, TCSANOW, &term);
		//setbuf(stdin, NULL);
	}
	#endif
	
	// Go to alternte screen buffer
	printf("\x1B[?1047h");
	
	gpCurrentFile = &gaFiles[0];
	
	UpdateDisplayFull();
	
	giProgramState = 1;
	
	while(giProgramState)
	{
		char	ch = 0;
		
		read(0, &ch, 1);
		
		if(ch == '\x1B')
		{
			read(0, &ch, 1);
			switch(ch)
			{
			case 'A':	CursorUp(gpCurrentFile);	break;
			case 'B':	CursorDown(gpCurrentFile);	break;
			case 'C':	CursorRight(gpCurrentFile);	break;
			case 'D':	CursorLeft(gpCurrentFile);	break;
			
			case '[':
				read(0, &ch, 1);
				switch(ch)
				{
				case 'A':	CursorUp(gpCurrentFile);	break;
				case 'B':	CursorDown(gpCurrentFile);	break;
				case 'C':	CursorRight(gpCurrentFile);	break;
				case 'D':	CursorLeft(gpCurrentFile);	break;
				default:
					printf("ch (\\x1B[) = %x\r", ch);
					fflush(stdout);
					break;
				}
				break;
			
			default:
				printf("ch (\\x1B) = %x\r", ch);
				fflush(stdout);
				break;
			}
			// TODO: Move
		}
		
		switch(ch)
		{
		case 'q':
			giProgramState = 0;
			break;
		}
	}

	return 0;
}

int OpenFile(tFile *Dest, const char *Path)
{
	FILE	*fp;
	char	*buffer;
	 int	i, j;
	 int	start;
	
	fp = fopen(Path, "r");
	if(!fp) {
		fprintf(stderr, "Unable to open %s\n", Path);
		return -1;
	}
	
	Dest->Filename = Path;
	
	fseek(fp, 0, SEEK_END);
	Dest->FileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	buffer = malloc(Dest->FileSize+1);
	fread(buffer, Dest->FileSize, 1, fp);
	fclose(fp);
	buffer[Dest->FileSize] = '\0';
	
	Dest->LineCount = 1;
	for( i = 0; i < Dest->FileSize; i ++ )
	{
		if( buffer[i] == '\n' )
			Dest->LineCount ++;
	}
	
	Dest->LineBuffer = malloc( Dest->LineCount * sizeof(char*) );
	start = 0;
	j = 0;
	for( i = 0; i < Dest->FileSize; i ++ )
	{
		if( buffer[i] == '\n' )
		{
			buffer[i] = '\0';
			Dest->LineBuffer[j] = strdup( &buffer[start] );
			start = i+1;
			j ++;
		}
	}
	Dest->LineBuffer[j] = strdup( &buffer[start] );
	
	free(buffer);
	
	Dest->FirstLine = 0;
	Dest->CurrentLine = 0;
	Dest->CurrentPos = 0;
	
	return 0;
}

void ShowLine(tFile *File, int LineNum, int X, int Y, int W)
{
	 int	j, k;
	char	*line;
	
	line = File->LineBuffer[LineNum];
	Term_SetPos(Y, X);
	printf("\x1B[2K");	// Clear line
	printf("%6i  ", LineNum+1);
	j = 8;
	for( k = 0; j < W-1 && line[k]; j++, k++ )
	{
		switch( line[k] )
		{
		case '\t':
			printf("\t");
			j += 8;
			j &= ~7;
			break;
		default:
			if( line[k] < ' ' || line[k] >= 0x7F )	continue;
			printf("%c", line[k]);
			break;
		}
	}
	
	for( ; j < W-1; j++ )
		printf(" ");
	printf("\n");
}

void ShowFile(tFile *File, int X, int Y, int W, int H)
{
	 int	i;
	Term_SetPos(Y, X);
	
	for( i = 0; i < H; i ++ )
	{
		if( File->FirstLine + i >= File->LineCount )	break;
		ShowLine( File, File->FirstLine + i, X, Y + i, W );
	}
	for( ; i < H; i++ )
		printf("\n");
}

void UpdateDisplayFull(void)
{
	#if USE_LOCAL
	{
		struct winsize ws;
		ioctl(0, TIOCGWINSZ, &ws);
		giTerminal_Width = ws.ws_col;
		giTerminal_Height = ws.ws_row;
	}
	#else
	giTerminal_Width = ioctl(1, TERM_IOCTL_WIDTH, NULL);
	giTerminal_Height = ioctl(1, TERM_IOCTL_HEIGHT, NULL);
	#endif
	
	printf("\x1B[2J");
	
	ShowFile(gpCurrentFile, 0, 0, giTerminal_Width, giTerminal_Height - 1);
	
	
	// Status Line
	UpdateDisplayStatus();
	
	Term_SetPos(
		gpCurrentFile->CurrentLine-gpCurrentFile->FirstLine,
		8 + gpCurrentFile->CurrentPos
		);
	fflush(stdout);
}

void UpdateDisplayLine(int Line)
{
	ShowLine(
		gpCurrentFile, Line,
		0, Line - gpCurrentFile->FirstLine,
		giTerminal_Width );
}

void UpdateDisplayStatus(void)
{
	int lastLine = gpCurrentFile->FirstLine + giTerminal_Height - 1;
	
	if( lastLine > gpCurrentFile->LineCount )
		lastLine = gpCurrentFile->LineCount;

	Term_SetPos( giTerminal_Height - 1, 0 );	
	printf("--- Line %i/%i (showing %i to %i)",
		gpCurrentFile->CurrentLine + 1, gpCurrentFile->LineCount,
		gpCurrentFile->FirstLine + 1, lastLine);
}

void UpdateCursorPosition(void)
{
	tFile	*File = gpCurrentFile;
	 int	screen_x, i;
	const char	*tmp;

	// Determine the position (handling tab characters)
	screen_x = 0;
	tmp = File->LineBuffer[File->CurrentLine];
	for( i = 0; tmp[i] && i < File->CurrentPos; i ++ )
	{
		switch(*tmp)
		{
		case '\t':	screen_x += giTabSize;	break;
		default:	screen_x ++;	break;
		}
	}

	Term_SetPos( File->CurrentLine - File->FirstLine, 8 + screen_x);
}

void CursorUp(tFile *File)
{
	if( File->CurrentLine <= 0 )
		return ;
	
	File->CurrentLine --;
	if( File->FirstLine > File->CurrentLine )
	{
		Term_ScrollUp(File->FirstLine - File->CurrentLine);
		File->FirstLine = File->CurrentLine;
//		UpdateDisplayLine(File->FirstLine + giTerminal_Height-2);
		UpdateDisplayLine(File->FirstLine);
//		UpdateDisplayFull();
	}
	else
	{
		UpdateDisplayLine(File->CurrentLine + 1);
		UpdateDisplayLine(File->CurrentLine);
	}
	UpdateDisplayStatus();
	
	UpdateCursorPosition();
}

void CursorDown(tFile *File)
{
	 int	threshold;
	// Bounds check
	if( File->CurrentLine + 1 >= File->LineCount )
		return ;
	
	File->CurrentLine ++;
	// Check if scroll is needed
	threshold = File->CurrentLine - (giTerminal_Height-2); 
	if( File->FirstLine < threshold )
	{
		Term_ScrollUp( File->FirstLine - threshold );
		File->FirstLine = threshold;
		UpdateDisplayLine(File->FirstLine + giTerminal_Height-2);
	}
	else
	{
		// Else, just update the previous and new lines (and status)
		UpdateDisplayLine(File->CurrentLine - 1);
		UpdateDisplayLine(File->CurrentLine);
	}
	UpdateDisplayStatus();
	
	UpdateCursorPosition();
}

void CursorLeft(tFile *File)
{
	if( File->CurrentPos > 0 )
	{
		File->CurrentPos --;
		UpdateDisplayLine(File->CurrentLine);
		UpdateDisplayStatus();
	}
	UpdateCursorPosition();
}

void CursorRight(tFile *File)
{
	if( File->LineBuffer[File->CurrentLine][File->CurrentPos+1] )
	{
		File->CurrentPos ++;
		UpdateDisplayLine(File->CurrentLine);
		UpdateDisplayStatus();
	}
	UpdateCursorPosition();
}

void Term_SetPos(int Row, int Col)
{
	printf("\x1B[%i;%iH", Row+1, Col+1);	// Set cursor
	fflush(stdout);
}

void Term_ScrollUp(int Count)
{
	printf("\x1B[r");
//	printf("\x1B[1;%ir", giTerminal_Height-1);
	if(Count < 0)
		printf("\x1B[%iM", -Count);
	else
		printf("\x1B[%iL", Count);
	fflush(stdout);
}
