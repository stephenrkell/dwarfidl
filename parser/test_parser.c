#include <stdio.h>
#include <assert.h>

#include "antlr3.h"
#include "antlr3defs.h"
#include "dwarfidlSimpleCLexer.h"
#include "dwarfidlSimpleCParser.h"

typedef ANTLR3_TOKEN_SOURCE TokenSource;
typedef ANTLR3_COMMON_TOKEN CommonToken;
typedef ANTLR3_INPUT_STREAM ANTLRInputStream;
typedef ANTLR3_COMMON_TOKEN_STREAM CommonTokenStream;
typedef ANTLR3_BASE_TREE Tree; 
typedef ANTLR3_COMMON_TREE CommonTree;

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
	
	pANTLR3_STRING s = tree->toStringTree(tree);
	
	printf("%s\n", (char*) s->chars);
	
	return 0;
}	
