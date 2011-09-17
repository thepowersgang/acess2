/*
 * SpiderScript Library
 * by John Hodge (thePowersGang)
 * 
 * bytecode_makefile.c
 * - Generate a bytecode file
 */
#include <stdlib.h>
#include "ast.h"
#include "bytecode_gen.h"
#include <stdio.h>
#include <string.h>

// === IMPORTS ===

// === PROTOTYPES ===

// === GLOBALS ===

// === CODE ===
int Bytecode_ConvertScript(tSpiderScript *Script, const char *DestFile)
{
	tStringList	strings = {0};
	tAST_Function	*fcn;
	FILE	*fp;
	 int	fcn_hdr_offset = 0;
	 int	fcn_count = 0;
	 int	strtab_ofs;

	void _put8(uint8_t val)
	{
		fwrite(&val, 1, 1, fp);
	}
	void _put32(uint32_t val)
	{
		_put8(val & 0xFF);
		_put8(val >> 8);
		_put8(val >> 16);
		_put8(val >> 24);
	}

	fp = fopen(DestFile, "wb");
	if(!fp)	return 1;
	
	// Create header
	fwrite("SSBC\r\n\xBC\x55", 8, 1, fp);
	_put32(0);	// Function count, to be filled
	_put32(0);	// String count
	_put32(0);	// String table offset
	// TODO: Variant info

	fcn_hdr_offset = ftell(fp);

	// Create function descriptors
	for(fcn = Script->Script->Functions; fcn; fcn = fcn->Next, fcn_count ++)
	{
		tAST_Node *arg;

		_put32( StringList_GetString(&strings, fcn->Name, strlen(fcn->Name)) );
		_put32( 0 );	// Code offset
		_put8( fcn->ReturnType );
		
		if(fcn->ArgumentCount > 255) {
			// ERROR: Too many args
			return 2;
		}
		_put8( fcn->ArgumentCount );

		// Argument types?
		for(arg = fcn->Arguments; arg; arg = arg->NextSibling)
		{
			_put32( StringList_GetString(&strings, arg->DefVar.Name, strlen(arg->DefVar.Name)) );
			_put8( arg->DefVar.DataType );
		}
	}

	// Put function code in
	for(fcn = Script->Script->Functions; fcn; fcn = fcn->Next)
	{
		char	*code;
		 int	len, code_pos;
		tBC_Function	*bc_fcn;
	
		// Fix header	
		code_pos = ftell(fp);
		fseek(fp, SEEK_SET, fcn_hdr_offset + 4);
		_put32( code_pos );
		fseek(fp, SEEK_SET, code_pos );

		fcn_hdr_offset += 4+4+1+1+(4+1)*fcn->ArgumentCount;
		
		// Write code
		bc_fcn = Bytecode_ConvertFunction(fcn);
		code = Bytecode_SerialiseFunction(bc_fcn, &len, &strings);
		Bytecode_DeleteFunction(bc_fcn);
		fwrite(code, len, 1, fp);
		free(code);
	}

	// String table
	strtab_ofs = ftell(fp);
	{
		 int	string_offset = strtab_ofs + (4+4)*strings.Count;
		tString	*str;
		// Array
		for(str = strings.Head; str; str = str->Next)
		{
			_put32(str->Length);
			_put32(string_offset);
			string_offset += str->Length + 1;
		}
		// Data
		for(str = strings.Head; str;)
		{
			tString	*nextstr = str->Next;
			fwrite(str->Data, str->Length, 1, fp);
			_put8(0);
			free(str);
			str = nextstr;
		}
		strings.Head = NULL;
		strings.Tail = NULL;
	}

	// Fix header
	fseek(fp, 8, SEEK_SET);
	_put32(fcn_count);
	_put32(strings.Count);
	_put32(strtab_ofs);

	fclose(fp);

	return 0;
}

