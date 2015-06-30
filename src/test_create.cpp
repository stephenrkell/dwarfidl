#include <stdio.h>
#include <assert.h>

#include <fstream>
#include <string>

#include "antlr3.h"
#include "antlr3defs.h"
#include "dwarfidlSimpleCLexer.h"
#include "dwarfidlSimpleCParser.h"

#include "dwarfidl/create.hpp"
#include "dwarfprint.hpp"

typedef ANTLR3_TOKEN_SOURCE TokenSource;
typedef ANTLR3_COMMON_TOKEN CommonToken;
typedef ANTLR3_INPUT_STREAM ANTLRInputStream;
typedef ANTLR3_COMMON_TOKEN_STREAM CommonTokenStream;
typedef ANTLR3_BASE_TREE Tree; 
typedef ANTLR3_COMMON_TREE CommonTree;

using namespace std;
using namespace dwarf;
using namespace dwarf::core;

using dwarfidl::create_dies;

int main(int argc, char **argv)
{
	assert(argc == 2);
	const char *filename = argv[1];

	pANTLR3_INPUT_STREAM in_fileobj = antlr3FileStreamNew((uint8_t *) filename,
        ANTLR3_ENC_UTF8);
	dwarfidlSimpleCLexer *lexer = dwarfidlSimpleCLexerNew(in_fileobj);
	CommonTokenStream *tokenStream = antlr3CommonTokenStreamSourceNew(
		ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
	dwarfidlSimpleCParser *parser = dwarfidlSimpleCParserNew(tokenStream); 
    dwarfidlSimpleCParser_toplevel_return ret = parser->toplevel(parser);
    Tree *tree = ret.tree;

	iterator_base iter = create_dies(tree);

	set<iterator_base> die_set;
	die_set.insert(iter);
	print_dies(cout, die_set);
	
	return 0;
}	
