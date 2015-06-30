#ifndef DWARFIDL_LANG_HPP_
#define DWARFIDL_LANG_HPP_

#define LEXER_INCLUDE "dwarfidlSimpleCLexer.h"
#define PARSER_INCLUDE "dwarfidlSimpleCParser.h"
#include <antlr3cxx/parser.hpp>

#include <string>

namespace dwarfidl
{
	using std::string;

	string unescape_ident(const string& s);
	string unescape_string_lit(const std::string& lit);
}

#endif
