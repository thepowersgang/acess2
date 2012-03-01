/*
 * Acess2 Kernel - Keyboard Character Mappings
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Core keyboard multiplexer
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

#define USE_KERNEL_MAGIC	1

// === IMPORTS ===
#if USE_KERNEL_MAGIC
extern void	Threads_ToggleTrace(int TID);
extern void	Threads_Dump(void);
extern void	Heap_Stats(void);
#endif

// === PROTOTYPES ===
 int	Keyboard_Install(char **Arguments);
void	Keyboard_Cleanup(void);
// - Internal
tKeymap	*Keyboard_LoadMap(const char *Name);
void	Keyboard_FreeMap(tKeymap *Keymap);
// - User side
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
 int	giKB_MagicAddress = 0;
 int	giKB_MagicAddressPos = 0;
#endif

// === CODE ===
int Keyboard_Install(char **Arguments)
{
	DevFS_AddDevice( &gKB_DevInfo );
	return 0;
}

void Keyboard_Cleanup(void)
{
	// TODO: Do I need this?
}

tKeymap *Keyboard_LoadMap(const char *Name)
{
	return NULL;
}

void Keyboard_FreeMap(tKeymap *Keymap)
{
}

// --- VFS Interface ---
static const char *csaIOCTL_NAMES[] = {DRV_IOCTLNAMES, DRV_KEYBAORD_IOCTLNAMES, NULL};
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

void Keyboard_RemoveInstance(tKeyboard *Instance)
{
	// TODO: Implement
	Log_Error("Keyboard", "TODO: Implement Keyboard_RemoveInstance");
}

void Keyboard_HandleKey(tKeyboard *Source, Uint32 HIDKeySym)
{
	 int	bPressed;
	Uint32	trans;
	Uint32	flag;
	Uint8	layer;
	
	bPressed = !(HIDKeySym & (1 << 31));
	HIDKeySym &= 0x7FFFFFFF;

	// Determine action
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
		
		// Get the action (Press, Refire or Release)
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
				flag = -1 ; // Do nothing
		}
	}

	// Translate \a State into layer
	// TODO: Support non-trivial layouts
	layer = !!(Source->State & 3);
	
	// Send raw symbol
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

	// --- Check for Kernel Magic Combos
	#if USE_KERNEL_MAGIC
	if(Source->KeyStates[KEYSYM_LEFTCTRL/8] & (1 << (KEYSYM_LEFTCTRL&7))
	&& Source->KeyStates[KEYSYM_LEFTALT/8]  & (1 << (KEYSYM_LEFTALT &7)) )
	{
		 int	val;
		switch(trans)
		{
		case '0': val = 0;  goto _av;	case '1': val = 1;  goto _av;
		case '2': val = 2;  goto _av;	case '3': val = 3;  goto _av;
		case '4': val = 4;  goto _av;	case '5': val = 5;  goto _av;
		case '6': val = 6;  goto _av;	case '7': val = 7;  goto _av;
		case '8': val = 8;  goto _av;	case '9': val = 9;  goto _av;
		case 'a': val = 10; goto _av;	case 'b': val = 11; goto _av;
		case 'c': val = 12; goto _av;	case 'd': val = 13; goto _av;
		case 'e': val = 14; goto _av;	case 'f': val = 15; goto _av;
		_av:
			if(giKB_MagicAddressPos == BITS/4)	break;
			giKB_MagicAddress |= (Uint)val << giKB_MagicAddressPos;
			giKB_MagicAddressPos ++;
			break;
		
		// Instruction Tracing
		case 't':
			Log("Toggle instruction tracing on %i\n", giKB_MagicAddress);
			Threads_ToggleTrace( giKB_MagicAddress );
			giKB_MagicAddress = 0;	giKB_MagicAddressPos = 0;
			return;
		
		// Thread List Dump
		case 'p':	Threads_Dump();	return;
		// Heap Statistics
		case 'h':	Heap_Stats();	return;
		// Dump Structure
		case 's':	return;
		}
	}
	#endif

}

