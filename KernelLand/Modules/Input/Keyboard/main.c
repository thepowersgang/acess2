/*
 * Acess2 Kernel - Keyboard Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Keyboard driver core
 *
 * TODO: Make the key transation code more general (for non-US layouts)
 * TODO: Support multiple virtual keyboards
 */
#define DEBUG	0
#define VERSION	VER2(1,0)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <Input/Keyboard/include/keyboard.h>
#include "keymap_int.h"
#include "layout_kbdus.h"
#include <debug_hooks.h>

#define USE_KERNEL_MAGIC	1

extern void	Validate_VirtualMemoryUsage(void);

// === PROTOTYPES ===
 int	Keyboard_Install(char **Arguments);
 int	Keyboard_Cleanup(void);
// - Internal
tKeymap	*Keyboard_LoadMap(const char *Name);
void	Keyboard_FreeMap(tKeymap *Keymap);
// - "User" side (Actually VT)
 int	Keyboard_IOCtl(tVFS_Node *Node, int ID, void *Data);
// - Device Side
tKeyboard *Keyboard_CreateInstance(int MaxSym, const char *Name);
void	Keyboard_RemoveInstance(tKeyboard *Instance);
void	Keyboard_HandleKey(tKeyboard *Source, Uint32 HIDKeySym);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Keyboard, Keyboard_Install, Keyboard_Cleanup, NULL);
tVFS_NodeType	gKB_NodeType = {
	.IOCtl = Keyboard_IOCtl
};
tDevFS_Driver	gKB_DevInfo = {
	NULL, "Keyboard",
	{ .Type = &gKB_NodeType }
};
#if USE_KERNEL_MAGIC
tDebugHook	gKB_DebugHookInfo;
#endif

// === CODE ===
/**
 * \brief Initialise the keyboard driver
 */
int Keyboard_Install(char **Arguments)
{
	/// - Register with DevFS
	DevFS_AddDevice( &gKB_DevInfo );
	return 0;
}

/**
 * \brief Pre-unload cleanup function
 */
int Keyboard_Cleanup(void)
{
	// TODO: Do I need this?
	return 0;
}

// --- Map Management ---
/**
 * \brief Load an arbitary keyboard map
 * \param Name	Keymap name (e.g. "en-us")
 * \return Keymap pointer
 */
tKeymap *Keyboard_int_LoadMap(const char *Name)
{
	Log_Warning("Keyboard", "TOD: Impliment Keyboard_int_LoadMap");
	return NULL;
}

/**
 * \brief Unload a keyboard map
 * \param Keymap	Keymap to unload/free
 */
void Keyboard_int_FreeMap(tKeymap *Keymap)
{
}

// --- VFS Interface ---
static const char *csaIOCTL_NAMES[] = {DRV_IOCTLNAMES, DRV_KEYBAORD_IOCTLNAMES, NULL};
/**
 * \brief Keyboard IOCtl
 */
int Keyboard_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	switch(Id)
	{
	BASE_IOCTLS(DRV_TYPE_KEYBOARD, "Keyboard", 0x100, csaIOCTL_NAMES);
	
	case KB_IOCTL_SETCALLBACK:
		if( Threads_GetUID() != 0 )	return -1;
		if( MM_IsUser( (tVAddr)Data ) )	return -1;
		if( Node->ImplInt )	return 0;	// Can only be set once
		Node->ImplInt = (Uint)Data;
		return 1;
	}
	return -1;
}

// --- Device Interface ---
/*
 * Create a new keyboard device instance
 * TODO: Allow linking to other VFS nodes
 * See Input/Keyboard/include/keyboard.h
 */
tKeyboard *Keyboard_CreateInstance(int MaxSym, const char *Name)
{
	tKeyboard	*ret;
	 int	sym_bitmap_size = (MaxSym + 7)/8;
	 int	string_size = strlen(Name) + 1;

	ret = malloc( sizeof(tKeyboard) + sym_bitmap_size + string_size );
	if( !ret ) {
		return NULL;
	}
	// Clear
	memset(ret, 0, sizeof(tKeyboard) + sym_bitmap_size );
	// Set name
	ret->Name = (char*)ret + sizeof(tKeyboard) + sym_bitmap_size;
	memcpy(ret->Name, Name, string_size);
	// Set node and default keymap
	ret->Node = &gKB_DevInfo.RootNode;
	ret->Keymap = &gKeymap_KBDUS;

	return ret;
}

/*
 * Destroy a keyboard instance
 * - See Input/Keyboard/include/keyboard.h
 */
void Keyboard_RemoveInstance(tKeyboard *Instance)
{
	// TODO: Implement
	Log_Error("Keyboard", "TODO: Implement Keyboard_RemoveInstance");
}

inline bool IsPressed(tKeyboard *Kb, Uint8 KeySym)
{
	return !!(Kb->KeyStates[KeySym/8] & (1 << (KeySym&7)));
}

/*
 * Handle a key press/release event
 * - See Input/Keyboard/include/keyboard.h
 */
void Keyboard_HandleKey(tKeyboard *Source, Uint32 HIDKeySym)
{
	 int	bPressed;
	Uint32	trans;
	Uint32	flag;
	Uint8	layer;
	
	if( !Source ) {
		Log_Error("Keyboard", "Passed NULL handle");
		return ;
	}
	if( !Source->Node ) {
		Log_Error("Keyboard", "Passed handle with NULL node");
		return ;
	}
	
	bPressed = !(HIDKeySym & (1 << 31));
	HIDKeySym &= 0x7FFFFFFF;

	// - Determine the action (Press, Release or Refire)
	{
		Uint8	mask = 1 << (HIDKeySym&7);
		 int	ofs = HIDKeySym / 8;
		 int	oldstate = !!(Source->KeyStates[ofs] & mask);
		
		// Get the state of all other devices attached
		 int	otherstate = 0;
		for( tKeyboard *kb = Source->Node->ImplPtr; kb; kb = kb->Next )
		{
			if( kb == Source )	continue ;
			if( kb->MaxKeysym <= HIDKeySym )	continue ;
			otherstate = otherstate || (kb->KeyStates[ofs] & mask);
		}
		
		// Update key state
		if( bPressed )
			Source->KeyStates[ ofs ] |= mask;
		else
			Source->KeyStates[ ofs ] &= ~mask;
		
		// Get the final flag
		if( bPressed )
		{
			if( !oldstate && !otherstate )
				flag = KEY_ACTION_PRESS; // Down
			else
				flag = KEY_ACTION_REFIRE; // Refire
		}
		else
		{
			if( !otherstate )
				flag = KEY_ACTION_RELEASE; // Up
			else
				flag = -1; // Do nothing
		}
	}

	// Translate \a State into layer
	// TODO: Support non-trivial layouts
	layer = !!(Source->State & 3);
	
	// Do translation
	if( flag == KEY_ACTION_RELEASE )
		trans = 0;
	else {	
		
		// Translate the keysym into a character
		if( layer >= Source->Keymap->nLayers )
			trans = 0;
		else if( HIDKeySym >= Source->Keymap->Layers[layer]->nSyms )
			trans = 0;
		else
			trans = Source->Keymap->Layers[layer]->Sym[HIDKeySym];
		// - No translation in \a layer, fall back to layer=0
		if(!trans && HIDKeySym < Source->Keymap->Layers[0]->nSyms)
			trans = Source->Keymap->Layers[0]->Sym[HIDKeySym];
	}

	// Call callback (only if the action is valid)
	if( flag != -1 )
	{
		tKeybardCallback Callback = (void*)Source->Node->ImplInt;
		Callback( HIDKeySym | KEY_ACTION_RAWSYM );
		Callback( flag | trans );
	}

	// TODO: Translate this into agnostic code
	switch( HIDKeySym )
	{
	case KEYSYM_LEFTSHIFT:
		if(bPressed) Source->State |= 1;
		else Source->State &= ~1;
		break;
	case KEYSYM_RIGHTSHIFT:
		if(bPressed) Source->State |= 2;
		else Source->State &= ~2;
		break;
	}

	// Magic debug hooks
	#if USE_KERNEL_MAGIC
	if(bPressed && IsPressed(Source, KEYSYM_LEFTCTRL) && IsPressed(Source, KEYSYM_LEFTALT))
	{
		if( trans == '~' ) {
			// TODO: Latch mode
		}
		else {
			char	str[5];	// utf8 only supports 8 bytes
			size_t	len = WriteUTF8((Uint8*)str, trans);
			str[len] = '\0';
			DebugHook_HandleInput(&gKB_DebugHookInfo, len, str);
		}
		return ;
	}
	#endif

	// Ctrl-Alt-Del == Reboot
	#if defined(ARCHDIR_is_x86) || defined(ARCHDIR_is_x86_64)
	if(bPressed
	  && (IsPressed(Source, KEYSYM_LEFTCTRL) || IsPressed(Source, KEYSYM_RIGHTCTRL))
	  && (IsPressed(Source, KEYSYM_LEFTALT)  || IsPressed(Source, KEYSYM_LEFTALT))
	  )
	{
		if( HIDKeySym == KEYSYM_DELETE )
		{
			// Trigger triple fault
			__asm__ __volatile__ ("lgdt (%esp) ; int $0x0");
			for(;;);
		}
	}
	#endif

}

