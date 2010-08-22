/*
 * 
 */
#ifndef _SPIDERSCRIPT_H_
#define _SPIDERSCRIPT_H_

#include <stdint.h>

#define ERRPTR	((void*)((intptr_t)0-1))

/**
 * \brief Opaque script handle
 */
typedef struct sSpiderScript	tSpiderScript;

/**
 * \brief Variant type
 */
typedef struct sSpiderVariant	tSpiderVariant;
typedef struct sSpiderFunction	tSpiderFunction;
typedef struct sSpiderValue	tSpiderValue;
typedef struct sSpiderObjectDef	tSpiderObjectDef;
typedef struct sSpiderObject	tSpiderObject;


/**
 * \brief SpiderScript Variable Datatypes
 */
enum eSpiderScript_DataTypes
{
	SS_DATATYPE_UNDEF,	//!< Undefined
	SS_DATATYPE_NULL,	//!< NULL (Probably will never be used)
	SS_DATATYPE_DYNAMIC,	//!< Dynamically typed variable (will this be used?)
	SS_DATATYPE_OBJECT,	//!< Opaque object reference
	SS_DATATYPE_ARRAY,	//!< Array
	SS_DATATYPE_INTEGER,	//!< Integer (64-bits)
	SS_DATATYPE_REAL,	//!< Real Number (double)
	SS_DATATYPE_STRING,	//!< String
	NUM_SS_DATATYPES
};

/**
 * \brief Variant of SpiderScript
 */
struct sSpiderVariant
{
	const char	*Name;	// Just for debug
	
	 int	bDyamicTyped;
	
	 int	NFunctions;	//!< Number of functions
	tSpiderFunction	*Functions;	//!< Functions
	
	 int	NConstants;	//!< Number of constants
	tSpiderValue	*Constants;	//!< Number of constants
};

/**
 * \brief SpiderScript data object
 */
struct sSpiderValue
{
	 int	Type;	//!< Variable type
	 int	ReferenceCount;	//!< Reference count
	
	union {
		uint64_t	Integer;	//!< Integer data
		double	Real;	//!< Real Number data
		/**
		 * \brief String data
		 */
		struct {
			 int	Length;	//!< Length
			char	Data[];	//!< Actual string (\a Length bytes)
		}	String;
		/**
		 * \brief Variable data
		 */
		struct {
			 int	Length;	//!< Length of the array
			tSpiderValue	*Items[];	//!< Array elements (\a Length long)
		}	Array;
		
		tSpiderObject	*Object;
	};
};

/**
 * \brief Object Definition
 * 
 * Internal representation of an arbitary object.
 */
struct sSpiderObjectDef
{
	/**
	 * \brief Construct an instance of the object
	 * \param NArgs	Number of arguments
	 * \param Args	Argument count
	 * \return Pointer to an object instance (which must be fully valid)
	 * \retval NULL	Invalid parameter (usually, actually just a NULL value)
	 * \retval ERRPTR	Invalid parameter count
	 */
	tSpiderObject	*(*Constructor)(int NArgs, tSpiderValue *Args);
	
	/**
	 * \brief Clean up and destroy the object
	 * \param This	Object instace
	 * \note The object pointer (\a This) should be invalidated and freed
	 *       by this function.
	 */
	void	(*Destructor)(tSpiderObject *This);
	
	 int	NAttributes;	//!< Number of attributes
	
	//! Attribute definitions
	struct {
		const char	*Name;	//!< Attribute Name
		 int	bReadOnly;	//!< Allow writes to the attribute?
	}	*AttributeDefs;
	
	
	 int	NMethods;	//!< Number of methods
	tSpiderFunction	*Methods;	//!< Method Definitions
};

/**
 * \brief Object Instance
 */
struct sSpiderObject
{
	tSpiderObjectDef	*Type;	//!< Object Type
	 int	NReferences;	//!< Number of references
	void	*OpaqueData;	//!< Pointer to the end of the \a Attributes array
	tSpiderValue	*Attributes[];	//!< Attribute Array
};

/**
 * \brief Represents a function avaliable to a script
 */
struct sSpiderFunction
{
	/**
	 * \brief Function name
	 */
	const char	*Name;
	/**
	 * \brief Function handler
	 */
	tSpiderValue	*(*Handler)(tSpiderScript *Script, int nParams, tSpiderValue **Parameters);
	/**
	 * \brief Argument types
	 * 
	 * Zero or -1 terminated array of \a eSpiderScript_DataTypes.
	 * If the final entry is zero, the function has a fixed number of
	 * parameters, if the final entry is -1, the function has a variable
	 * number of arguments.
	 */
	 int	*ArgTypes;	// Zero (or -1) terminated array of parameter types
};


// === FUNCTIONS ===
/**
 * \brief Parse a file into a script
 * \param Variant	Variant structure
 * \param Filename	File to parse
 * \return Script suitable for execution
 */
extern tSpiderScript	*SpiderScript_ParseFile(tSpiderVariant *Variant, const char *Filename);
/**
 * \brief Execute a method from a script
 * \param Script	Script to run
 * \param Function	Name of function to run ("" for the 'main')
 * \return Return value
 */
extern tSpiderValue	*SpiderScript_ExecuteMethod(tSpiderScript *Script,
	const char *Function,
	int NArguments, tSpiderValue **Arguments
	);

/**
 * \brief Free a script
 */
extern void	SpiderScript_Free(tSpiderScript *Script);

#endif
