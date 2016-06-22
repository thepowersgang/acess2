/*
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

enum eToken {
	TOK_EOF,
	TOK_COMMENT,
	TOK_NEWLINE,
	TOK_BRACE_OPEN,
	TOK_BRACE_CLOSE,
	TOK_IDENT,
	TOK_STRING,
};

struct sLexer {
	const char* filename;
	FILE*	file;
	unsigned int cur_line;
	enum eToken	last_tok;
	char*	string;
};
typedef struct sLexer	tLexer;

struct sOutState {
	unsigned int	next_inode_idx;
	FILE*	out_c;
	FILE*	out_ldopts;
	
	//FILE*	out_dep;
	char**	dependencies;
	unsigned int	num_dependencies;
};
typedef struct sOutState	tOutState;

struct sDirEnt {
	unsigned int	inode;
	char	name[];
};
typedef struct sDirEnt tDirEnt;
struct sDirEnts {
	unsigned int	num_ents;
	struct sDirEnt**	ents;
};
typedef struct sDirEnts	tDirEnts;

tLexer	Lex_Init(const char* filename, FILE* fp);
void	Lex_Advance(struct sLexer* lex);
void	Lex_Consume(struct sLexer* lex);
char*	Lex_ConsumeYield(struct sLexer* lex);
void	Lex_Error(struct sLexer* lex, const char* message);
void	Lex_Expect(struct sLexer* lex, enum eToken exp);

void	ProcessDir(struct sLexer* lex, tOutState* os, unsigned int inode, enum eToken end);
void	AddFile(tOutState* os, tDirEnts* dents, bool is_opt, const char* name, const char* path);
char*	strdup(const char* s);

int main(int argc, const char *argv[])
{
	const char* infile = NULL;
	const char* outfile_c = NULL;
	const char* outfile_dep = NULL;
	const char* outfile_ldopts = NULL;
	
	for(int arg_i = 1; arg_i < argc; arg_i ++)
	{
		const char* arg = argv[arg_i];
		if( *arg != '-' ) {
			if( infile == NULL )
				infile = arg;
			else if( outfile_c == NULL )
				outfile_c = arg;
			else if( outfile_ldopts == NULL )
				outfile_ldopts = arg;
			else if( outfile_dep == NULL )
				outfile_dep = arg;
			else {
				fprintf(stderr, "Unexpected free argument\n");
				return 1;
			}
		}
		else {
			fprintf(stderr, "Unexpected argument\n");
			return 1;
		}
	}
	
	FILE* fp = fopen(infile, "r");
	if( !fp ) {
		perror("Unable to open input file");
		return 1;
	}
	struct sLexer lex = Lex_Init(infile, fp);
	
	tOutState os = {
		1,
		fopen(outfile_c, "w"),
		fopen(outfile_ldopts, "w"),
		//fopen(outfile_dep, "w"),
		NULL, 0,
		};
	
	fprintf(os.out_c,
		"/*"
		" * Acess2 InitRD\n"
		" * InitRD Data\n"
		" * Generated ???\n"
		" */\n"
		"#include \"initrd.h\"\n"
		);

	
	fprintf(os.out_ldopts, "--format binary\n");
	
	ProcessDir(&lex, &os, 0, TOK_EOF);
	
	// --- Finalize the C file (node list)
	fprintf(os.out_c, "tVFS_Node * const gInitRD_FileList[] = {");
	for(unsigned int i = 0; i < os.next_inode_idx; i ++ )
		fprintf(os.out_c, "&INITRD_%u, ", i);
	fprintf(os.out_c, "};\n");
	
	// --- Close running output files
	fclose(os.out_c);
	fclose(os.out_ldopts);
	
	// --- Write the dependency file
	FILE*	depfile = fopen(outfile_dep, "w");
	assert(depfile);
	fprintf(depfile, "%s:", outfile_c);
	for(unsigned int i = 0; i < os.num_dependencies; i ++)
		fprintf(depfile, " %s", os.dependencies[i]);
	fprintf(depfile, "\n");
	for(unsigned int i = 0; i < os.num_dependencies; i ++) {
		fprintf(depfile, "%s:\n", os.dependencies[i]);
		free(os.dependencies[i]);
	}
	free(os.dependencies);
	fclose(depfile);
	
	return 0;
}

void add_dir_ent(tDirEnts* dents, const char* name, unsigned int inode) {
	tDirEnt* de = malloc( sizeof(tDirEnt) + strlen(name) + 1 );
	de->inode = inode;
	strcpy(de->name, name);
	
	dents->num_ents += 1;
	dents->ents = realloc( dents->ents, sizeof(tDirEnt*) * dents->num_ents );
	assert(dents->ents);
	dents->ents[dents->num_ents-1] = de;
}

void ProcessDir(struct sLexer* lex, tOutState* os, unsigned int inode, enum eToken end)
{
	tDirEnts	dents = { 0, NULL };
	while( lex->last_tok != end )
	{
		while( lex->last_tok == TOK_NEWLINE ) {
			Lex_Consume(lex);	
		}
		if( lex->last_tok == end )
			break ;
		Lex_Expect(lex, TOK_IDENT);
		
		if( strcmp(lex->string, "Dir") == 0 ) {
			Lex_Consume(lex);
			Lex_Expect(lex, TOK_STRING);
			char* name = Lex_ConsumeYield(lex);
			Lex_Expect(lex, TOK_BRACE_OPEN);
			Lex_Consume(lex);

			unsigned int child_inode = os->next_inode_idx ++;
			
			ProcessDir(lex, os, child_inode, TOK_BRACE_CLOSE);
			
			add_dir_ent(&dents, name, child_inode);
			
			free(name);
		}
		else if( strcmp(lex->string, "File") == 0 || strcmp(lex->string, "OptFile") == 0 ) {
			bool is_optional = (lex->string[0] == 'O');
			Lex_Consume(lex);
			Lex_Expect(lex, TOK_STRING);
			char* path = Lex_ConsumeYield(lex);
			char* name = NULL;
			if( lex->last_tok == TOK_STRING ) {
				name = path;
				path = Lex_ConsumeYield(lex);
			}
			else {
				const char* pos = strrchr(path, '/');
				name = strdup(pos ? pos + 1 : path);
			}
			Lex_Expect(lex, TOK_NEWLINE);
			
			AddFile(os, &dents, is_optional, name, path);
			
			free(name);
			free(path);
		}
		else {
			Lex_Error(lex, "Unexpected entry");
		}
	}

	fprintf(os->out_c, "tInitRD_File INITRD_%u_entries[] = {", inode);
	for( unsigned int i = 0; i < dents.num_ents; i ++ ) {
		fprintf(os->out_c, "{\"%s\",&INITRD_%u}, ", dents.ents[i]->name, dents.ents[i]->inode);
		free(dents.ents[i]);
	}
	free(dents.ents);
	fprintf(os->out_c, "};\n");
	fprintf(os->out_c,
		"tVFS_Node INITRD_%u = {"
		"	.NumACLs = 1,"
		"	.ACLs = &gVFS_ACL_EveryoneRX,"
		"	.Flags = VFS_FFLAG_DIRECTORY,"
		"	.Size = %u,"
		"	.Inode = %u,"
		"	.ImplPtr = INITRD_%u_entries,"
		"	.Type = &gInitRD_DirType"
		"};\n\n"
		, inode, dents.num_ents, inode, inode
		);
	Lex_Consume(lex);
}

size_t expand_path(char* out, const char* in) {
	size_t	rv = 0;
	
	#define PUTC(v)	do { if(out) out[rv] = (v); rv += 1; } while(0)
	while(*in)
	{
		if(in[0] == '_' && in[1] == '_') {
			//const char* ARCHDIR = "x86";
			const char* ARCHDIR = getenv("ARCH");//"x86";
			if( strncmp(in, "__EXT__", 4+3) == 0 ) {
				in += 4+3;
				
				const char* s = "../../../../Externals/Output/";
				if(out)	strcpy(out+rv, s);
				rv += strlen(s);
				if(out) strcpy(out+rv, ARCHDIR);
				rv += strlen(ARCHDIR);
			}
			else if( strncmp(in, "__BIN__", 4+3) == 0 ) {
				in += 4+3;
				
				const char* s = "../../../../Usermode/Output/";
				if(out)	strcpy(out+rv, s);
				rv += strlen(s);
				if(out) strcpy(out+rv, ARCHDIR);
				rv += strlen(ARCHDIR);
			}
			else if( strncmp(in, "__FS__", 4+2) == 0 ) {
				in += 4+2;
				const char* s = "../../../../Usermode/Filesystem";
				if(out)	strcpy(out+rv, s);
				rv += strlen(s);
			}
			else if( strncmp(in, "__SRC__", 4+3) == 0 ) {
				in += 4+3;
				const char* s = "../../../..";
				if(out)	strcpy(out+rv, s);
				rv += strlen(s);
			}
			else {
				fprintf(stderr, "Encountered unknown replacement at '%s'\n", in);
				exit(1);
			}
		}
		else {
			PUTC(*in);
			in ++;
		}
	}
	if(out)	out[rv] = '\0';
	
	return rv;
}
size_t ld_mangle(char* out, const char* in) {
	size_t	rv = 0;
	while(*in)
	{
		switch(*in)
		{
		case '_': case '$':
		case '0' ... '9':
		case 'a' ... 'z':
		case 'A' ... 'Z':
			if(out)	out[rv] = *in;
			rv ++;
			break;
		default:
			if(out)	out[rv] = '_';
			rv ++;
			break;	
		}
		in ++;
	}
	if(out)	out[rv] = '\0';
	return rv;
}

void AddFile(tOutState* os, tDirEnts* dents, bool is_opt, const char* name, const char* path)
{
	char* realpath;
	{
		size_t len = expand_path(NULL, path);
		realpath = malloc(len+1);
		expand_path(realpath, path);
	}
	
	unsigned int file_size = 0;
	{
		FILE* fp = fopen(realpath, "r");
		if( !fp ) {
			if( is_opt ) {
				free(realpath);
				return ;
			}
			else {
				fprintf(stderr, "Couldn't open '%s': %s\n", realpath, strerror(errno));
				exit(1);
			}
		}
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		fclose(fp);
	}
	
	unsigned int node_idx = os->next_inode_idx ++;

	fprintf(os->out_ldopts, "%s\n", realpath);
	
	char* binary_sym = malloc(strlen(realpath)+1);
	ld_mangle(binary_sym, realpath);
	
	fprintf(os->out_c, "extern Uint8 _binary_%s_start[];\n", binary_sym);
	fprintf(os->out_c,
		"tVFS_Node INITRD_%i = {"
		"	.NumACLs = 1,"
		"	.ACLs = &gVFS_ACL_EveryoneRX,"
		"	.Flags = 0,"
		"	.Size = %u,"
		"	.Inode = %i,"
		"	.ImplPtr = _binary_%s_start,"
		"	.Type = &gInitRD_FileType"
		"};\n"
		, node_idx, file_size, node_idx, binary_sym
		);
	free(binary_sym);
	
	os->num_dependencies += 1;
	os->dependencies = realloc( os->dependencies, sizeof(*os->dependencies) * os->num_dependencies );
	assert(os->dependencies);
	os->dependencies[os->num_dependencies-1] = realpath;
	//free(realpath);

	add_dir_ent(dents, name, node_idx);
}

enum eToken get_token_int(FILE* ifp,  char** string)
{
	int ch = fgetc(ifp);
	
	while( isblank(ch) ) {
		if( ch == '\n' )
			return TOK_NEWLINE;
		ch = fgetc(ifp);
	}
	
	if( ch < 0 ) {
		return TOK_EOF;
	}
	
	switch(ch)
	{
	case '\n':
		return TOK_NEWLINE;
	case '#':
		while( (ch = fgetc(ifp)) != '\n' )
			;
		ungetc(ch, ifp);
		return TOK_COMMENT;
	case 'a' ... 'z':
	case 'A' ... 'Z': {
		size_t len = 0;
		char *s = NULL;
		while( isalnum(ch) ) {
			len += 1;
			s = realloc(s, len+1);
			assert(s);
			s[len-1] = ch;
			
			ch = fgetc(ifp);
		}
		s[len] = '\0';
		*string = s;
		return TOK_IDENT; }
	case '"': {
		size_t len = 0;
		char *s = malloc(1);
		while((ch = fgetc(ifp)) != '"')
		{
			len += 1;
			s = realloc(s, len+1);
			assert(s);
			s[len-1] = ch;
		}
		s[len] = '\0';
		*string = s;
		return TOK_STRING;
		}
	case '{':
		return TOK_BRACE_OPEN;
	case '}':
		return TOK_BRACE_CLOSE;
	default:
		fprintf(stderr, "ERROR: Unexpected character '%c'\n", ch);
		exit(1);
	}
}

tLexer Lex_Init(const char* filename, FILE* fp)
{
	tLexer	rv;
	rv.filename = filename;
	rv.file = fp;
	rv.string = NULL;
	rv.cur_line = 0;
	rv.last_tok = TOK_NEWLINE;
	Lex_Advance(&rv);
	return rv;
}
void Lex_Advance(struct sLexer* lex)
{
	do {
		assert(lex->string == NULL);
		if( lex->last_tok == TOK_NEWLINE ) {
			lex->cur_line += 1;
		}
		lex->last_tok = get_token_int(lex->file, &lex->string);
	} while( lex->last_tok == TOK_COMMENT );
}
void Lex_Consume(struct sLexer* lex)
{
	free(lex->string);
	lex->string = NULL;
	Lex_Advance(lex);
}
char* Lex_ConsumeYield(struct sLexer* lex)
{
	char* rv = lex->string;
	lex->string = NULL;
	Lex_Advance(lex);
	return rv;
}
void Lex_Error(struct sLexer* lex, const char* string)
{
	(void)lex;
	fprintf(stderr, "%s:%u: Parse error: %s\n", lex->filename, lex->cur_line, string);
	exit(1);
}
void Lex_Expect(struct sLexer* lex, enum eToken exp)
{
	if( lex->last_tok != exp ) {
		fprintf(stderr, "%s:%u: Unexpected token %i \"%s\", expected %i\n", lex->filename, lex->cur_line, lex->last_tok, lex->string, exp);
		exit(1);
	}
}

char* strdup(const char* s)
{
	char* rv = malloc(strlen(s) + 1);
	if( rv == NULL )
		return NULL;
	strcpy(rv, s);
	return rv;
}

