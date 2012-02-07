/*
 * Acess2 Kernel
 * - By John Hodge
 *
 * include/keyvalue.h
 * - Key/Value pair parsing
 */
#ifndef _ACESS_KEYVALUE_H_
#define _ACESS_KEYVALUE_H_

typedef struct sKeyVal_ParseRules	tKeyVal_ParseRules;
typedef struct sKeyVal_int_Rule	tKeyVal_int_Rule;
typedef void (*tKeyVal_UnkCb)(char *String);
typedef void (*tKeyVal_KeyCb)(const char *Key, char *Value);

/**
 * \brief Handling rule for a key
 */
struct sKeyVal_int_Rule
{
	const char	*Key;
	const char	*Type;	// Acess printf format, with 'F' being a tKeyVal_KeyCb
	void	*Data;
};

struct sKeyVal_ParseRules
{
	/**
	 * \brief Function to call when no match is found
	 */
	tKeyVal_UnkCb	Unknown;
	tKeyVal_int_Rule	Rules[];
};

/**
 * \brief Parse a NULL terminated list of strings as Key/Value pairs
 * \param Rules	Parsing rules
 * \param Strings	Input string list
 */
extern int	KeyVal_ParseNull(tKeyVal_ParseRules *Rules, char **Strings);

#endif

