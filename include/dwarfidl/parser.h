#ifndef DWARFIDL_PARSER_H_
#define DWARFIDL_PARSER_H_

#include "dwarfidlNewCLexer.h"
#include "dwarfidlNewCParser.h"

/* HACK for compatibility while we have the two versions of the 
 * language floating around -- this header gets you the new,
 * footprints-enabled one. */
#define dwarfidlCLexer dwarfidlNewCLexer
#define dwarfidlCLexerNew dwarfidlNewCLexerNew
#define dwarfidlCParser dwarfidlNewCParser
#define dwarfidlCParserNew dwarfidlNewCParserNew
#define dwarfidlCParser_expression_return dwarfidlNewCParser_expression_return
#endif
