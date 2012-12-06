/*
 * Acess Text Editor (ATE)
 * - By John Hodge (thePowersGang)
 *
 * strings.c
 * - String Localisation
 */

struct keyval_str
{
	const char	*key;
	const char	*val;
};

const struct keyval_str	gaDisplayStrings[] = {
	{"BtnNew", "New"},
	{"BtnOpen", "Open"},
	{"BtnSave", "Save"},
	{"BtnUndo", "Undo"}
	};

