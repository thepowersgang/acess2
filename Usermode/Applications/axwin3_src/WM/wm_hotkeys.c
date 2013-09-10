/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * wm_hotkeys.c
 * - Hotkey and key shortcut code
 */
#include <common.h>
#include <wm_internals.h>
#include <wm_messages.h>
#include <wm_hotkeys.h>
#include <string.h>

#define true	1
#define false	0

#define MAX_STATE_SCANCODE	256

// === GOBALS ===
char	gbWM_HasBeenKeyDown = true;
uint8_t	gWM_KeyStates[MAX_STATE_SCANCODE/8];
tHotkey	*gpWM_Hotkeys;
tHotkeyTarget	*gpWM_HotkeyTargets;
tHotkeyTarget	*gpWM_HotkeyTargets_Last;

// === PROTOTYPES ===
static void	_SetKey(uint32_t sc);
static void	_UnsetKey(uint32_t sc);
static int	_IsKeySet(uint32_t sc);
void	WM_Hotkey_FireEvent(const char *Target);

// === CODE ===
void WM_Hotkey_Register(int nKeys, uint32_t *Keys, const char *ActionName)
{
	// TODO: Duplicate detection
	
	// Create new structure
	tHotkey	*h = malloc(sizeof(tHotkey) + nKeys * sizeof(uint32_t) + strlen(ActionName) + 1);
	h->nKeys = nKeys;
	h->Target = (void*)( h->Keys + nKeys );
	strcpy((char*)h->Target, ActionName);
	memcpy(h->Keys, Keys, nKeys * sizeof(uint32_t));
	
	h->Next = gpWM_Hotkeys;
	gpWM_Hotkeys = h;
}

void WM_Hotkey_RegisterAction(const char *ActionName, tWindow *Target, uint16_t Index)
{
	// Check for a duplicate registration
	for( tHotkeyTarget *t = gpWM_HotkeyTargets; t; t = t->Next )
	{
		if( strcmp(t->Name, ActionName) != 0 )
			continue ;
		// Duplicate!
		return ;
	}

	// Create new structure
	tHotkeyTarget	*t = malloc(sizeof(tHotkeyTarget) + strlen(ActionName) + 1);
	t->Name = (void*)(t + 1);
	strcpy((char*)t->Name, ActionName);
	t->Window = Target;
	t->Index = Index;
	t->Next = NULL;

	// TODO: Register to be informed when the window dies/closes?

	// Append to list
	if( gpWM_HotkeyTargets ) {
		gpWM_HotkeyTargets_Last->Next = t;
	}
	else {
		gpWM_HotkeyTargets = t;
	}
	gpWM_HotkeyTargets_Last = t;
}

void WM_Hotkey_KeyDown(uint32_t Scancode)
{
	_SetKey(Scancode);
	gbWM_HasBeenKeyDown = true;
}

void WM_Hotkey_KeyUp(uint32_t Scancode)
{
	_UnsetKey(Scancode);

	// Ensure that hotkeys are triggered on the longest sequence
	// (so Win-Shift-R doesn't trigger Win-R if shift is released)
	if( !gbWM_HasBeenKeyDown )
		return ;

	for( tHotkey *hk = gpWM_Hotkeys; hk; hk = hk->Next )
	{
		int i;
		for( i = 0; i < hk->nKeys; i ++ )
		{
			if( hk->Keys[i] == Scancode )	continue ;
			if( _IsKeySet(hk->Keys[i]) )	continue ;
			break;
		}
		//_SysDebug("%i/%i satisfied for %s", i, hk->nKeys, hk->Target);
		if( i != hk->nKeys )
			continue ;
		
		// Fire shortcut
		WM_Hotkey_FireEvent(hk->Target);

		break;
	}
	
	gbWM_HasBeenKeyDown = false;
}

void WM_Hotkey_FireEvent(const char *Target)
{
//	_SysDebug("WM_Hotkey_FireEvent: (%s)", Target);
	// - Internal events (Alt-Tab, Close, Maximize, etc...)
	// TODO: Internal event handling
	
	// - Application registered events
	for( tHotkeyTarget *t = gpWM_HotkeyTargets, *prev=NULL; t; t = t->Next )
	{
		if( strcmp(t->Name, Target) != 0 )
			continue ;

		struct sWndMsg_Hotkey	info = {.ID=t->Index};
		WM_SendMessage(NULL, t->Window, WNDMSG_HOTKEY, sizeof(info), &info);

		// Sort the list by most-recently-used
		if(prev != NULL) {
			prev->Next = t->Next;
			t->Next = gpWM_HotkeyTargets;
			gpWM_HotkeyTargets = t;
		}

		return ;
	}
}

static void _SetKey(uint32_t sc)
{
//	_SysDebug("_SetKey: (%x)", sc);
	if( sc >= MAX_STATE_SCANCODE )	return;
	gWM_KeyStates[sc/8] |= 1 << (sc % 8);
}
static void _UnsetKey(uint32_t sc)
{
//	_SysDebug("_UnsetKey: (%x)", sc);
	if( sc >= MAX_STATE_SCANCODE )	return;
	gWM_KeyStates[sc/8] &= ~(1 << (sc % 8));
}
static int _IsKeySet(uint32_t sc)
{
	if( sc >= MAX_STATE_SCANCODE )	return 0;

	return !!(gWM_KeyStates[sc/8] & (1 << (sc % 8)));
}

