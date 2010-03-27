#ifndef EDI_OBJECTS_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#define EDI_OBJECTS_H

/*! \file edi_objects.h
 * \brief The header file for basic EDI types and the object system.
 *
 * This file contains declarations of EDI's primitive data types, as well as structures and functions for with the object system.
 * It represents these data structures and algorithms:
 * 
 * 	DATA STRUCTURE: THE CLASS LIST - EDI implementing runtime's must keep an internal list of classes implemented by the runtime
 * 	and separate lists of classes implemented by each driver.  Whoever implements a class is said to "own" that class.  The
 * 	internal format of this list is up to the runtime coders, but it must be possible to recreate the original list of
 * 	edi_class_declaration structures the driver declared to the runtime from it.  This list is declared to the runtime in an
 * 	initialization function in the header edi.h.  The object_class member of an edi_object_metadata structure must point to that
 * 	object's class's entry in this list.
 * 	
 * 	ALGORITHM AND DATA STRUCTURE: CLASSES AND INHERITANCE - Classes are described using edi_class_declaration_t structures and
 * 	follow very simple rules.  All data is private and EDI provides no way to access instance data, so there are no member
 * 	variable declarations.  However, if the data isn't memory-protected (for example, driver data on the driver heap) EDI allows
 * 	the possibility of pointer access to data, since runtime and driver coders could make use of that behavior.  Classes may have
 * 	one ancestor by declaring so in their class declaration structure, and if child methods are different then parent methods
 * 	the children always override their parents.  An EDI runtime must also be able to check the existence and ownership of a given
 * 	class given its name in an edi_string_t.
 * 	
 * 	ALGORITHM: OBJECT CREATION AND DESTRUCTION - An EDI runtime should be able to call the constructor of a named class, put the
 * 	resulting object_pointer into an edi_object_metadata_t and return that structure.  The runtime should also be able to call an
 * 	object's class's destructor when given a pointer to a valid edi_metadata_t for an already-existing object.  Data equivalent
 * 	to an edi_object_metadata_t should also be tracked by the runtime for every object in existence in case of sudden EDI shutdown
 * 	(see edi.h).
 *
 * 	ALGORITHM: RUNTIME TYPE INFORMATION - When passed the data_pointer member of an edi_object_metadata_t to a valid object, an
 * 	EDI runtime must be able to return an edi_string_t containing the name of that object's class and to return function_pointers
 * 	to methods when the required information to find the correct method is given by calling a class's method getting function.*/

/* If the EDI headers are linked with the standard C library, they use its type definitions.  Otherwise, equivalent definitions are
 * made.*/
#if __STDC_VERSION__ == 199901L
# include <stdbool.h>
# include <stdint.h>
#else
# ifndef NULL
#  define	NULL	((void*)0)
# endif
typedef unsigned char bool;
# define true 1
# define false 0
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef long long int64_t;
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
typedef unsigned long long	uint64_t;
#endif

/*! \brief Define a variable in the header
 */
#ifdef EDI_MAIN_FILE
# define EDI_DEFVAR
#else
# define EDI_DEFVAR	extern
#endif

/*! \brief A pointer to the in-memory instance of an object.
 *
 * This type is sized just like a general C pointer type (whatever*) for the target architecture.  It's passed as a first parameter
 * to all methods, thus allowing EDI classes to be implemented as C++ classes and providing some protection from confusing objects
 * with normal pointers.  Equivalent to a C++ this pointer or an Object Pascal Self argument. */
typedef void *object_pointer;
/*! \brief A basic pointer type pointing to arbitrary data in an arbitrary location. */
typedef void *data_pointer;
/*! \brief A basic function pointer type.
 *
 * A pointer to a piece of code which can be called and return to its caller, used to distinguish between pointers to code and
 * pointers to data.  Its size is hardware-dependent. */
typedef void (*function_pointer)(void);
/*! \brief The length of an EDI string without its null character. */
#define EDI_STRING_LENGTH 31
/*! \brief A type representing a 31-character long string with a terminating NULL character at the end.  All of EDI uses this type
 * for strings.
 *
 * A null-terminated string type which stores characters in int8s.  It allows for 31 characters in each string, with the final
 * character being the NULL terminator.  Functions which use this type must check that its final character is NULL, a string which
 * doesn't not have this property is invalid and insecure.  I (the author of EDI) know and understand that this form of a string
 * suffers from C programmer's disease, but I can't use anything else without either making string use far buggier or dragging
 * everyone onto a better language than C.  */
typedef int8_t edi_string_t[0x20];
/*! \brief A type representing a pointer form of #edi_string_t suitable for function returns
 */
typedef int8_t *edi_string_ptr_t;

/*! \var EDI_BASE_TYPES
 * \brief A constant array of edi_string_t's holding every available EDI primitive type. */
/*! \var EDI_TYPE_MODIFIERS
 * \brief A constant array of edi_string_t's holding available modifiers for EDI primitive types. */
#ifdef IMPLEMENTING_EDI
 const edi_string_t EDI_BASE_TYPES[9] = {"void","bool","int8_t","int16_t","int32_t","int64_t","function_pointer","intreg","edi_string_t"};
 const edi_string_t EDI_TYPE_MODIFIERS[2] = {"pointer","unsigned"};
#else
 //extern const edi_string_t EDI_BASE_TYPES[9] = {"void","bool","int8_t","int16_t","int32_t","int64_t","function_pointer","intreg", "edi_string_t"};
 //extern const edi_string_t EDI_TYPE_MODIFIERS[2] = {"pointer","unsigned"};
 extern const edi_string_t EDI_BASE_TYPES[9];
 extern const edi_string_t EDI_TYPE_MODIFIERS[2];
#endif

/*! \struct edi_object_metadata_t
 * \brief A packed structure holding all data to identify an object to the EDI object system. */
typedef struct {
	/*! \brief Points to the instance data of the object represented by this structure.
	 *
	 * An object_pointer to the object this structure refers to.  The this pointer, so to speak. */
	object_pointer object;
	/*! \brief Points the internal record kept by the runtime describing the object's class.
	 *
	 * Points to wherever the runtime has stored the class data this object was built from.  The class data doesn't need to be
	 * readable to the driver, and so this pointer can point to an arbitrary runtime-reachable location. */
	data_pointer object_class;
} edi_object_metadata_t;

/*! \struct edi_variable_declaration_t
 * \brief The data structure used to describe a variable declaration to the EDI object system.
 *
 * The data structure used to describe a variable declaration to the EDI object system.  The context of the declaration depends on
 * where the data structure appears, ie: alone, in a class declaration, in a parameter list, etc. */
typedef struct {
	/*! \brief The type of the declared variable. 
	 *
	 * The type of the variable, which must be a valid EDI primitive type as specified in the constant EDI_BASE_TYPES and
	 * possibly modified by a modifier specified in the constant EDI_TYPE_MODIFIERS. */
	edi_string_t type;
	/*! \brief The name of the declared variable. */
	edi_string_t name;
	/*! \brief Number of array entries if this variable is an array declaration. 
	 *
	 * An int32_t specifying the number of variables of 'type' in the array 'name'.  For a single variable this value should
	 * simply be set to 1, for values greater than 1 a packed array of contiguous variables is being declared, and a value of 0
	 * is invalid. */
	int32_t array_length;
} edi_variable_declaration_t;

/*! \struct edi_function_declaration_t
 * \brief The data structure used to declare a function to the EDI object system. */
typedef struct {
	/*! \brief The return type of the function.  The same type rules which govern variable definitions apply here. */
	edi_string_t return_type;
	/*! \brief The name of the declared function. */
	edi_string_t name;
	/*! \brief The version number of the function, used to tell different implementations of the same function apart. */
	uint32_t version;
	/*! \brief The number of arguments passed to the function.
	 *
	 * The number of entries in the member arguments that the object system should care about.  Caring about less misses
	 * parameters to functions, caring about more results in buffer overflows. */
	uint32_t num_arguments;
	/*! \brief An array of the declared function's arguments.
	 *
	 * A pointer to an array num_arguments long containing edi_variable_declaration_t's for each argument to the declared
	 * function.*/
	edi_variable_declaration_t *arguments;
	/*!\brief A pointer to the declared function's code in memory. */
	function_pointer code;
} edi_function_declaration_t;

/*! \brief A pointer to a function for constructing instances of a class.
 *
 * A pointer to a function which takes no parameters and returns an object_pointer pointing to the newly made instance of a class.
 * It is the constructor's responsibility to allocate memory for the new object.  Each EDI class needs one of these. */
typedef object_pointer (*edi_constructor_t)(void);
/*! \brief A pointer to a function for destroying instances of a class.
 *
 * A pointer to a function which takes an object_pointer as a parameter and returns void.  This is the destructor counterpart to a
 * class's edi_constructor_t, it destroys the object pointed to by its parameter and frees the object's memory.  Every class must
 * have one */
typedef void (*edi_destructor_t)(object_pointer);

/*! \brief Information the driver must give the runtime about its classes so EDI can construct them and call their methods.
 *
 * A structure used to declare a class to an EDI runtime so instances of it can be constructed by the EDI object system. */
typedef struct {
	/*! \brief The name of the class declared by the structure. */
	edi_string_t name;
	/*! \brief The version of the class.  This number is used to tell identically named but differently
	 * implemented classes apart.*/
	uint32_t version;
	/*! \brief The number of methods in the 'methods' function declaration array. */
	uint32_t num_methods;
	/*! \brief An array of edi_function_declaration_t declaring the methods of this class. */
	edi_function_declaration_t *methods;
	/*! \brief Allocates the memory for a new object of the declared class and constructs the object.  Absolutely required.*/
	edi_constructor_t constructor;
	/*! \brief Destroys the given object of the declared class and frees its memory. Absolutely required. */
	edi_destructor_t destructor;
	/*! \brief A pointer to another EDI class declaration structure specifying the declared class's parent class. 
	 *
	 * Points to a parent class declared in another class declaration.  It can be NULL to mean this class has no parent. */
	struct edi_class_declaration_t *parent;
} edi_class_declaration_t;

/*! \brief Checks the existence of the named class.
 *
 * This checks for the existence on THE CLASS LIST of the class named by its edi_string_t parameter and returns a signed int32_t.  If
 * the class isn't found (ie: it doesn't exist as far as EDI is concerned) -1 is returned, if the class is owned by the driver
 * (implemented by the driver and declared to the runtime by the driver) 0, and if the class is owned by the runtime (implemented by
 * the runtime) 1. */
int32_t check_class_existence(edi_string_t class_name);
/*! \brief Constructs an object of the named class and returns its object_pointer and a data_pointer to its class data.
 *
 * Given a valid class name in an edi_string_t this function constructs the specified class and returns an edi_metadata_t describing
 * the new object as detailed in OBJECT CREATION AND DESTRUCTION.  If the construction fails it returns a structure full of NULL
 * pointers. */
edi_object_metadata_t construct_object(edi_string_t class_name);
/*! \brief Destroys the given object using its class data.
 *
 * As specified in OBJECT CREATION AND DESTRUCTION this function should destroy an object when given its valid edi_metadata_t.  The
 * destruction is accomplished by calling the class's destructor. */
void destroy_object(edi_object_metadata_t object);
/*! \brief Obtains a function pointer to a named method of a given class. 
 *
 * When given a valid data_pointer object_class from an edi_object_metadata_t and an edi_string_t representing the name of the
 * desired method retrieves a function_pointer to the method's machine code in memory.  If the desired method isn't found, NULL is
 * returned. */
function_pointer get_method_by_name(data_pointer object_class,edi_string_t method_name);
/*! \brief Obtains a function pointer to a method given by a declaration of the given class if the class's method matches the
 * declaration. 
 *
 * Works just like get_method_by_name(), but by giving an edi_function_declaration_t for the desired method instead of just its name.
 * Performs detailed checking against THE CLASS LIST to make sure that the method returned exactly matches the declaration passed
 * in. */
function_pointer get_method_by_declaration(data_pointer object_class,edi_function_declaration_t declaration);

/* Runtime typing information. */
/*! \brief Returns the name of the class specified by a pointer to class data. 
 *
 * Given the data_pointer to an object's class data as stored in an edi_object_metadata_t retrieves the name of the object's class
 * and returns it in an edi_string_t. */
edi_string_ptr_t get_object_class(data_pointer object_class);
/*! \brief Returns the name of a class's parent class.
 *
 * When given an edi_string_t with a class name in it, returns another edi_string_t containing the name of the class's parent, or an
 * empty string. */
edi_string_ptr_t get_class_parent(edi_string_t some_class);
/*! \brief Returns the internal class data of a named class (if it exists) or NULL.
 *
 * When given an edi_string_t with a valid class name in it, returns a pointer to the runtime's internal class data for that class.
 * Otherwise, it returns NULL. */
data_pointer get_internal_class(edi_string_t some_class);

#endif 
