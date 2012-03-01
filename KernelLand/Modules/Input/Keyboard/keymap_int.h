/*
 * Acess2 Kernel - Keyboard Character Mappings
 * - By John Hodge (thePowersGang)
 *
 * keymap.h
 * - Core keymap "library" header
 */
#ifndef _KEYMAP__KEYMAP_INT_H_
#define _KEYMAP__KEYMAP_INT_H_

typedef struct sKeymap	tKeymap;
typedef struct sKeymapLayer	tKeymapLayer;

struct sKeymapLayer
{
	 int	nSyms;
	Uint32	Sym[];
};

struct sKeymap
{
	char	*Name;
	 int	nLayers;
	tKeymapLayer	*Layers[];
};

struct sKeyboard
{
	struct sKeyboard	*Next;
	char	*Name;
	struct sVFS_Node	*Node;

	tKeymap	*Keymap;	

	Uint32	State;
	 int	MaxKeysym;
	Uint8	KeyStates[];
};

#endif

