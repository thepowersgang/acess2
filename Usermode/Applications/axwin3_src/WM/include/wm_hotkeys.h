/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * wm_hotkeys.h
 * - Hotkey and key shortcut code
 */
#ifndef _WM_HOTKEYS_H_
#define _WM_HOTKEYS_H_

typedef struct sHotkey	tHotkey;
typedef struct sHotkeyTarget	tHotkeyTarget;

struct sHotkey
{
	tHotkey	*Next;
	
	const char	*Target;

	 int	nKeys;
	uint32_t	Keys[];
};

struct sHotkeyTarget
{
	struct sHotkeyTarget	*Next;

	const char	*Name;
	
	tWindow	*Window;
	 int	Index;
};


extern void	WM_Hotkey_Register(int nKeys, uint32_t *Keys, const char *ActionName);
extern void	WM_Hotkey_KeyDown(uint32_t Scancode);
extern void	WM_Hotkey_KeyUp(uint32_t Scancode);

#endif

