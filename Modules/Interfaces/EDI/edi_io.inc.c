/*
 * AcessOS EDI Interface
 * - IO Port Class
 * 
 * By John Hodge (thePowersGang)
 * 
 * This file has been released into the public domain.
 * You are free to use it as you wish.
 */
#include "edi/edi.h"

// === TYPES ===
typedef struct {
	uint16_t	State;	// 0: Unallocated, 1: Allocated, 2: Initialised, (Bit 0x8000 set if in heap)
	uint16_t	Num;
} tEdiPort;

// === GLOBALS ===
#define	NUM_PREALLOC_PORTS	128
tEdiPort	gEdi_PortObjects[NUM_PREALLOC_PORTS];

// === FUNCTIONS ===
/**
 * \fn object_pointer Edi_Int_IO_Construct(void)
 * \brief Creates a new IO Port Object
 * \return	Pointer to object
 */
object_pointer Edi_Int_IO_Construct(void)
{
	tEdiPort	*ret;
	 int	i;
	// Search for a free preallocated port
	for( i = 0; i < NUM_PREALLOC_PORTS; i ++ )
	{
		if(gEdi_PortObjects[i].State)	continue;
		gEdi_PortObjects[i].State = 1;
		gEdi_PortObjects[i].Num = 0;
		return &gEdi_PortObjects[i];
	}
	// Else, use heap space
	ret = malloc( sizeof(tEdiPort) );
	ret->State = 0x8001;
	ret->Num = 0;
	return ret;
}

/**
 * \fn void Edi_Int_IO_Destruct(object_pointer Object)
 * \brief Destruct an IO Port Object
 * \param Object	Object to destroy
 */
void Edi_Int_IO_Destruct(object_pointer Object)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object,);
	obj = GET_DATA(Object);
	
	if(obj->State & 0x8000) {	// If in heap, free
		free(Object);
	} else {	// Otherwise, mark as unallocated
		obj->State = 0;
	}
}

/**
 * \fn int32_t	Edi_Int_IO_InitPort(object_pointer Object, uint16_t Port)
 * \brief Initialises an IO Port
 * \param Object	Object Pointer (this)
 * \param Port	Port Number to use
 */
int32_t	Edi_Int_IO_InitPort(object_pointer Object, uint16_t Port)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	
	if( !obj->State )	return 0;
	obj->Num = Port;
	obj->State &= ~0x3FFF;
	obj->State |= 2;	// Set initialised flag
	return 1;
}

/**
 * \fn uint16_t Edi_Int_IO_GetPortNum(object_pointer Object)
 * \brief Returns the port number associated with the object
 * \param Object	Port Object to get number from
 * \return Port Number
 */
uint16_t Edi_Int_IO_GetPortNum(object_pointer Object)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	// Check if valid
	if( !obj->State )	return 0;
	// Return Port No
	return obj->Num;
}

/**
 * \fn int32_t Edi_Int_IO_ReadByte(object_pointer Object, uint8_t *out)
 * \brief Read a byte from an IO port
 * \param Object	Port Object
 * \param out	Pointer to put read data
 */
int32_t Edi_Int_IO_ReadByte(object_pointer Object, uint8_t *out)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "inb %%dx, %%al" : "=a" (*out) : "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_ReadWord(object_pointer Object, uint16_t *out)
 * \brief Read a word from an IO port
 * \param Object	Port Object
 * \param out	Pointer to put read data
 */
int32_t Edi_Int_IO_ReadWord(object_pointer Object, uint16_t *out)
{
	
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "inw %%dx, %%ax" : "=a" (*out) : "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_ReadDWord(object_pointer Object, uint32_t *out)
 * \brief Read a double word from an IO port
 * \param Object	Port Object
 * \param out	Pointer to put read data
 */
int32_t Edi_Int_IO_ReadDWord(object_pointer Object, uint32_t *out)
{
	
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "inl %%dx, %%eax" : "=a" (*out) : "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_ReadQWord(object_pointer Object, uint64_t *out)
 * \brief Read a quad word from an IO port
 * \param Object	Port Object
 * \param out	Pointer to put read data
 */
int32_t Edi_Int_IO_ReadQWord(object_pointer Object, uint64_t *out)
{
	uint32_t	*out32 = (uint32_t*)out;
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "inl %%dx, %%eax" : "=a" (*out32) : "d" ( obj->Num ) );
	__asm__ __volatile__ ( "inl %%dx, %%eax" : "=a" (*(out32+1)) : "d" ( obj->Num+4 ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_ReadString(object_pointer Object, uint32_t Length, uint8_t *out)
 * \brief Read a byte from an IO port
 * \param Object	Port Object
 * \param Length	Number of bytes to read
 * \param out	Pointer to put read data
 */
int32_t Edi_Int_IO_ReadString(object_pointer Object, uint32_t Length, uint8_t *out)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "rep insb" : : "c" (Length), "D" (out), "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_WriteByte(object_pointer Object, uint8_t in)
 * \brief Write a byte from an IO port
 * \param Object	Port Object
 * \param in	Data to write
 */
int32_t Edi_Int_IO_WriteByte(object_pointer Object, uint8_t in)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "outb %%al, %%dx" : : "a" (in), "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_WriteWord(object_pointer Object, uint16_t in)
 * \brief Write a word from an IO port
 * \param Object	Port Object
 * \param in	Data to write
 */
int32_t Edi_Int_IO_WriteWord(object_pointer Object, uint16_t in)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "outw %%ax, %%dx" : : "a" (in), "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_WriteDWord(object_pointer Object, uint32_t in)
 * \brief Write a double word from an IO port
 * \param Object	Port Object
 * \param in	Data to write
 */
int32_t Edi_Int_IO_WriteDWord(object_pointer Object, uint32_t in)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "outl %%eax, %%dx" : : "a" (in), "d" ( obj->Num ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_WriteQWord(object_pointer Object, uint64_t in)
 * \brief Write a quad word from an IO port
 * \param Object	Port Object
 * \param in	Data to write
 */
int32_t Edi_Int_IO_WriteQWord(object_pointer Object, uint64_t in)
{
	uint32_t	*in32 = (uint32_t*)&in;
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "outl %%eax, %%dx" : : "a" (*in32), "d" ( obj->Num ) );
	__asm__ __volatile__ ( "outl %%eax, %%dx" : : "a" (*(in32+1)), "d" ( obj->Num+4 ) );
	
	return 1;
}

/**
 * \fn int32_t Edi_Int_IO_WriteString(object_pointer Object, uint32_t Length, uint8_t *in)
 * \brief Read a byte from an IO port
 * \param Object	Port Object
 * \param Length	Number of bytes to write
 * \param in	Pointer to of data to write
 */
int32_t Edi_Int_IO_WriteString(object_pointer Object, uint32_t Length, uint8_t *in)
{
	tEdiPort	*obj;
	// Get Data Pointer
	VALIDATE_PTR(Object, 0);
	obj = GET_DATA(Object);
	if( !obj->State )	return 0;
	if( obj->State & 1 )	return -1;	// Unintialised
	
	__asm__ __volatile__ ( "rep outsb" : : "c" (Length), "D" (in), "d" ( obj->Num ) );
	
	return 1;
}

// === CLASS DECLARATION ===
/*static edi_variable_declaration_t	*scEdi_Int_Variables_IO[] = {
	{
		{"pointer", "port_object", 0},
		{"uint16_t", "port", 0}
	},
	{
		{"pointer", "port_object", 0}
	},
	{
		{"pointer", "port_object", 0},
		{"pointer int8_t", "out", 0}
	}
};*/
static edi_function_declaration_t	scEdi_Int_Functions_IO[] = {
		{"int32_t", "init_io_port", 1, 2, NULL, //scEdi_Int_Variables_IO[0],
			(function_pointer)Edi_Int_IO_InitPort
			},
		{"uint16_t", "get_port_number", 1, 1, NULL, //scEdi_Int_Variables_IO[1],
			(function_pointer)Edi_Int_IO_GetPortNum
			},
		{"int32_t", "read_byte_io_port", 1, 2, NULL, //scEdi_Int_Variables_IO[2],
			(function_pointer)Edi_Int_IO_ReadByte
			},
		{"int32_t", "read_word_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"pointer int16_t", "out", 0}
			}*/,
			(function_pointer)Edi_Int_IO_ReadWord
			},
		{"int32_t", "read_long_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"pointer int32_t", "out", 0}
			}*/,
			(function_pointer)Edi_Int_IO_ReadDWord
			},
		{"int32_t", "read_longlong_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"pointer int64_t", "out", 0}
			}*/,
			(function_pointer)Edi_Int_IO_ReadQWord
			},
		{"int32_t", "read_string_io_port", 1, 3, NULL/*{
				{"pointer", "port_object", 0},
				{"int32_T", "data_length", 0},
				{"pointer int64_t", "out", 0}
			}*/,
			(function_pointer)Edi_Int_IO_ReadString
			},
			
		{"int32_t", "write_byte_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"int8_t", "in", 0}
			}*/,
			(function_pointer)Edi_Int_IO_WriteByte},
		{"int32_t", "write_word_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"int16_t", "in", 0}
			}*/,
			(function_pointer)Edi_Int_IO_WriteWord},
		{"int32_t", "write_long_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"int32_t", "in", 0}
			}*/,
			(function_pointer)Edi_Int_IO_WriteDWord},
		{"int32_t", "write_longlong_io_port", 1, 2, NULL/*{
				{"pointer", "port_object", 0},
				{"int64_t", "in", 0}
			}*/,
			(function_pointer)Edi_Int_IO_WriteQWord}
	};
static edi_class_declaration_t	scEdi_Int_Class_IO = 
	{
		IO_PORT_CLASS, 1, 12,
		scEdi_Int_Functions_IO,
		Edi_Int_IO_Construct,
		Edi_Int_IO_Destruct,
		NULL
	};
