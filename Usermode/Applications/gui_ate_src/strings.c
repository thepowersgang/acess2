/*
 * Acess Text Editor (ATE)
 * - By John Hodge (thePowersGang)
 *
 * strings.c
 * - String Localisation
 */
#include "strings.h"
#include <string.h>

struct keyval_str
{
	const char	*key;
	const char	*val;
};

const struct keyval_str	gaDisplayStrings[] = {
	{"BtnNew", "New"},
	{"BtnOpen", "Open"},
	{"BtnSave", "Save"},
	{"BtnClose", "Close"},
	{"BtnUndo", "Undo"},
	{"BtnRedo", "Redo"},
	{"BtnCut", "Cut"},
	{"BtnCopy", "Copy"},
	{"BtnPaste", "Paste"},
	{"BtnSearch", "Search"},
	{"BtnReplace", "Replace"},
	};
const int	ciNumDisplayStrings = sizeof(gaDisplayStrings)/sizeof(gaDisplayStrings[0]);

const char *getstr(const char *key)
{
	 int	i;
	for(i = 0; i < ciNumDisplayStrings; i ++)
	{
		if( strcmp(key, gaDisplayStrings[i].key) == 0 )
			return gaDisplayStrings[i].val;
	}
	return "-nostr-";
}

const char *getimg(const char *key)
{
	return "";
}
