/*
 */
#include "input.h"
#include "window.h"
#include "server.h"
#include <readline.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
// === PROTOTYPES ===
void	Input_FillSelect(int *nfds, fd_set *rfds);
void	Input_HandleSelect(int nfds, const fd_set *rfds);
 int	ParseUserCommand(char *String);

// === GLOBALS ===
tReadline	*gpInput_ReadlineInfo;

// === CODE ===
void Input_FillSelect(int *nfds, fd_set *rfds)
{
	if( !gpInput_ReadlineInfo ) {
		gpInput_ReadlineInfo = Readline_Init(1);
	}
	
	FD_SET(0, rfds);
	if(*nfds < 0+1)
		*nfds = 0+1;
}

void Input_HandleSelect(int nfds, const fd_set *rfds)
{
	// User input
	if(FD_ISSET(0, rfds))
	{
		char	*cmd = Readline_NonBlock(gpInput_ReadlineInfo);
		if( cmd )
		{
			if( cmd[0] )
			{
				ParseUserCommand(cmd);
			}
			free(cmd);
			// Prompt
			SetCursorPos(giTerminal_Height-1, 1);
			printf("\x1B[2K");	// Clear line
			printf("[%s]", Window_GetName(NULL));
		}
	}
}

void Cmd_join(char *ArgString)
{
	 int	pos=0;
	char	*channel_name = GetValue(ArgString, &pos);
	
	tServer	*srv = Window_GetServer(NULL);
	
	if( srv )
	{
		Windows_SwitchTo( Window_Create(srv, channel_name) );
		Redraw_Screen();
		Server_SendCommand(srv, "JOIN :%s", channel_name);
	}
}

void Cmd_quit(char *ArgString)
{
	const char *quit_message = ArgString;
	if( quit_message == NULL || quit_message[0] == '\0' )
		quit_message = "/quit - Acess2 IRC Client";
	
	Servers_CloseAll(quit_message);
	
	Exit(NULL);	// NULL = user requested
}

void Cmd_window(char *ArgString)
{
	 int	pos = 0;
	char	*window_id = GetValue(ArgString, &pos);
	 int	window_num = atoi(window_id);
	
	if( window_num > 0 )
	{
		// Get `window_num`th window
		tWindow	*win = Windows_GetByIndex(window_num-1);
		if( win )
		{
			Windows_SwitchTo( win );
		}
		else
		{
			// Otherwise, silently ignore
		}
	}
	else
	{
		window_num = 1;
		for( tWindow *win; (win = Windows_GetByIndex(window_num-1)); window_num ++ )
		{
			Window_AppendMessage(WINDOW_STATUS, MSG_CLASS_CLIENT, NULL, "%i: %s", window_num, Window_GetName(win));
		}
	}
}

const struct {
	const char *Name;
	void	(*Fcn)(char *ArgString);
} caCommands[] = {
	{"join", Cmd_join},
	{"quit", Cmd_quit},
	{"window", Cmd_window},
	{"win",    Cmd_window},
	{"w",      Cmd_window},
};
const int ciNumCommands = sizeof(caCommands)/sizeof(caCommands[0]);

/**
 * \brief Handle a line from the prompt
 */
int ParseUserCommand(char *String)
{
	if( String[0] == '/' )
	{
		char	*command;
		 int	pos = 0;
		
		command = GetValue(String, &pos)+1;

		// TODO: Prefix matches
		 int	cmdIdx = -1;
		for( int i = 0; i < ciNumCommands; i ++ )
		{
			if( strcmp(command, caCommands[i].Name) == 0 ) {
				cmdIdx = i;
				break;
			}
		}
		if( cmdIdx != -1 ) {
			caCommands[cmdIdx].Fcn(String+pos);
		}
		else
		{
			Window_AppendMessage(WINDOW_STATUS, MSG_CLASS_CLIENT, NULL, "Unknown command %s", command);
		}
	}
	else
	{
		// Message
		// - Only send if server is valid and window name is non-empty
		tServer	*srv = Window_GetServer(NULL);
		if( srv && Window_IsChat(NULL) ) {
			Window_AppendMessage(NULL, MSG_CLASS_MESSAGE, Server_GetNick(srv), "%s", String);
			Server_SendCommand(srv, "PRIVMSG %s :%s\n", Window_GetName(NULL), String);
		}
	}
	
	return 0;
}
